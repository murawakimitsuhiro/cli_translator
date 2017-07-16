#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

int main(int argc, const char * argv[]) {
    int is_debug = 0;

    const char *text;
    const char *yandex_api_key;

    char msg[1000];
    char *host = "translate.yandex.net";
    char *path = "/api/v1.5/tr.json/translate";
    char *query = "?lang=ja-en&format=plain&text=";
    int   port = 80;

    struct sockaddr_in server;
    struct addrinfo hints;  //こっち側から送るaddrinfo構造体
    struct addrinfo *res;   //getaddrinfoの結果を格納するaddrinfo構造体

    //引数受け取り
    int i;
    for (i=0; i < argc; i++) {
      if (i == 1) {
        text = argv[i];
      } else if (strncmp(argv[i], "-d", 2) == 0) {
        is_debug= 1;
      } else if (strncmp(argv[i], "-h", 2) == 0) {
        printf("translate help\n");
        printf("      1st arg please japanese\n");
        printf("      -h view help\n");
        printf("      -d view debug log\n");
        return 0;
      }
    }

    //環境変数からYANDEX_API_KEYを取得
    yandex_api_key = getenv("YANDEX_API_KEY");
    if (is_debug) {
      printf("apiのキーは%s", yandex_api_key);
    }

    // IPアドレスの解決
    memset(&hints, 0, sizeof(hints));   //hintsをゼロ埋め
    hints.ai_family   = AF_INET;  //IPV4を設定
    hints.ai_socktype = SOCK_STREAM;  //socketのタイプを設定(SOCK_STREAMは双方向の接続されたバイトストリームを提供)
    char *service = "https";  //https

    int err = 0;
    //addrinfo(インターネットアドレスが格納されて構造体)を取得する. getaddrinfoはIPv4とIPv6の違いを意識せず使える。
    if ((err = getaddrinfo(host, service, &hints, &res)) != 0) {
      fprintf(stderr, "Fail to resolve ip address - %d\n", err);
      return EXIT_FAILURE;
    }

    //ソケットを作る
    int mysocket;
    if ((mysocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
      fprintf(stderr, "Fail to create a socket.\n");
      return EXIT_FAILURE;
    }

    //サーバにコネクションをはる
    if (connect(mysocket, res->ai_addr, res->ai_addrlen) != 0) {
      printf("Connection error.\n");
      return EXIT_FAILURE;
    }

    //作ったコネクションをHTTPSで送信できるようにする.
    SSL *ssl;
    SSL_CTX *ctx;

    SSL_load_error_strings(); //エラー文書読み込み
    SSL_library_init();   //初期化

    //SSL_CTX 構造体生成
    ctx = SSL_CTX_new(SSLv23_client_method());
    //SSL構造体を生成
    ssl = SSL_new(ctx);
    //コネクションしてあるsocketと紐づける
    err = SSL_set_fd(ssl, mysocket);
    //sslでコネクションを飛ばす
    SSL_connect(ssl);

    if (is_debug) {
      printf("Conntect to %s\n", host);
    }

    //HTTPの各種パラメータを設定
    sprintf(msg, "GET %s%s%s&key=%s HTTP/1.0\r\nHost: %s\r\n\r\n", path, query, text, yandex_api_key, host);
    if (is_debug) {
      printf("HTTPS sended :  %s\n", msg);
    }

    //http接続でいうwriteと同じもの(strlen = 文字列の長さを取得する)
    SSL_write(ssl, msg, strlen(msg));

    //帰って来た値を読み込む
    int buf_size = 512;
    char buf[buf_size];
    int read_size;

    //帰って来た値から必要な値を抜き出す
    char json_key[] = "text"; 
    char *sp;
    char result[128];

    //受信が終わるまでループしながらbufに入れていく
    do {
      read_size = SSL_read(ssl, buf, buf_size);
      sp = strstr(buf, json_key); //jsonKeyが戻り値に含まれているかどうか
      strcpy(result, sp); //含まれている場合、result配列にコピー
      if (is_debug) {
        write(1, buf, read_size);
      }
    } while(read_size > 0);

    if (is_debug) {
      printf("\n\n result : ");
    }

    //整形の処理
    char end_str = ']'; //終わりの文字列を定義
    char *res_end_pt; //終わりの文字列のpointor
    int end_point;  //終わりの文字列がresultの何番目か

    res_end_pt = strchr(result, end_str); //終わりの文字列を検索
    end_point = res_end_pt - result - 1;  //resultの何番目で終わらせるべきかを代入

    char t[64]; 
    strncpy(t, result+8, end_point); //strの先頭+8の位置からend_point文字をtにコピー
    t[end_point - 8] = '\0';            //取り出した文字数分の最後に'\0'を入れる)
    printf("%s\n", t);  //結果表示

    //あと処理
    SSL_shutdown(ssl);  //sslを閉じる
    SSL_free(ssl);    //ssl構造体を解放
    SSL_CTX_free(ctx);  //ctx構造体を解放
    ERR_free_strings(); //SSL_load_error_strings() で確保した領域を開放する

    close(mysocket);  //閉じて終わり

    return EXIT_SUCCESS;
}

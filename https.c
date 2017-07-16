#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

int main(void) {
    char msg[1000];
    char *host = "translate.yandex.net";
    char *path = "/api/v1.5/tr.json/translate";
    char *query = "?key=trnsl.1.1.20170715T123719Z.40387c7fe671307f.5024e55b8a40ceeb39a0e1efdc5992117513de75&text=こんにちは世界&lang=ja-en&format=plain";
    int   port = 80;

    struct sockaddr_in server;
    struct addrinfo hints;  //こっち側から送るaddrinfo構造体
    struct addrinfo *res; //getaddrinfoの結果を格納するaddrinfo構造体

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

    printf("Conntect to %s\n", host);

    //HTTPの各種パラメータを設定
    sprintf(msg, "GET %s%s HTTP/1.0\r\nHost: %s\r\n\r\n", path, query, host);
    printf("HTTPS sended :  %s\n", msg);

    //http接続でいうwriteと同じもの(strlen = 文字列の長さを取得する)
    SSL_write(ssl, msg, strlen(msg));


    //帰って来た値を読み込む
    int buf_size = 512;//256;
    char buf[buf_size];
    int read_size;

    //受信が終わるまでループしながらbufに入れていく
    do {
      read_size = SSL_read(ssl, buf, buf_size);
      write(1, buf, read_size);
    } while(read_size > 0);

    //あと処理
    SSL_shutdown(ssl);  //sslを閉じる
    SSL_free(ssl);    //ssl構造体を解放
    SSL_CTX_free(ctx);  //ctx構造体を解放
    ERR_free_strings(); //SSL_load_error_strings() で確保した領域を開放する

    close(mysocket);  //閉じて終わり

    return EXIT_SUCCESS;
}

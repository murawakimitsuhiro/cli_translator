#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <unistd.h>

#define BUF_LEN 256

struct URL {
    char host[BUF_LEN];
    char path[BUF_LEN];
    char query[BUF_LEN];
    char fragment[BUF_LEN];
    unsigned short port;
};

/**
   * @param urlStr URLテキスト
    */
void parseURL(const char *urlStr, struct URL *url, char **error);

int main(int argc, char **argv) {

    // ソケットのためのファイルディスクリプタ
    int s;

    // IPアドレスの解決
    struct addrinfo hints, *res;
    struct in_addr addr;
    int err;

    // サーバに送るHTTPプロトコル用バッファ
    char send_buf[BUF_LEN];

    struct URL url = {
            "web.sfc.keio.ac.jp", "/~t17792mm/index.html", 80
        };

    // URLが指定されていたら
    if (argc > 1) {
            char *error = NULL;
            parseURL(argv[1], &url, &error);
    
            if (error) {
                        printf("%s\n", error);
                        return 1;
                    }
        }

    printf("https://%s%s%s を取得します。\n\n", url.host, url.path, url.query);

    // 0クリア
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family   = AF_INET;

    char *serviceType = "https";

    if ((err = getaddrinfo(url.host, serviceType, &hints, &res)) != 0) {
            printf("error %d\n", err);
            return 1;
        }

    // ソケット生成
    if ((s = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
            fprintf(stderr, "ソケットの生成に失敗しました。\n");
            return 1;
        }

    // サーバに接続
    if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            fprintf(stderr, "connectに失敗しました。\n");
            return 1;
        }

    // HTTPプロトコルの開始 ＆サーバに送信
    sprintf(send_buf, "GET %s%s HTTP/1.0\r\n", url.path, url.query);
    printf("これが送られています%s", send_buf);
    write(s, send_buf, strlen(send_buf));

    sprintf(send_buf, "Host: %s:%d\r\n", url.host, url.port);
    write(s, send_buf, strlen(send_buf));

    sprintf(send_buf, "\r\n");
    write(s, send_buf, strlen(send_buf));

    // 受信が終わるまでループ
    while(1) {
            char buf[BUF_LEN];
            int read_size;
            read_size = read(s, buf, BUF_LEN);
    
            if (read_size > 0) {
                        write(1, buf, read_size);
                    }
            else {
                        break;
                    }
        }

    // ソケットを閉じる
    close(s);

    return 0;
}


void parseURL(const char *urlStr, struct URL *url, char **error) {
    char host_path[BUF_LEN];

    if (strlen(urlStr) > BUF_LEN - 1) {
            *error = "URLが長過ぎます。\n";
            return;
        }

    // https://から始まる文字列で
    // sscanfが成功して
    // https://の後に何か文字列が存在するなら
    if (strstr(urlStr, "https://")              &&
                sscanf(urlStr, "https://%s", host_path) &&
                        strcmp(urlStr, "https://")) {
    
            char *p = NULL;
    
            p = strchr(host_path, '#');
            if (p != NULL) {
                        strcpy(url->fragment, p);
                        *p = '\0';
                    }
    
            p = strchr(host_path, '?');
            if (p != NULL) {
                        strcpy(url->query, p);
                        *p = '\0';
                    }
    
            p = strchr(host_path, '/');
            if (p != NULL) {
                        strcpy(url->path, p);
                        *p = '\0';
                    }
    
            strcpy(url->host, host_path);
    
            // ホスト名の部分に":"が含まれていたら
            p = strchr(url->host, ':');
            if (p != NULL) {
                        // ポート番号を取得
                        url->port = atoi(p + 1);
            
                        // 数字ではない（atoiが失敗）か、0だったら
                        // ポート番号は80に決め打ち
                        if (url->port <= 0) {
                                        url->port = 80;
                                    }
            
                        // 終端文字で空にする
                        *p = '\0';
                    }
            else {
                        url->port = 80;
                    }
        }
    else {
            *error = "URLはhttps://host/pathの形式で指定してください。\n";
            return;
        }
}

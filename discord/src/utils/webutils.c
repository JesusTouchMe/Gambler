// Copyright 2025 JesusTouchMe

#define _GNU_SOURCE

#include "utils/webutils.h"

#include "config.h"

#include <openssl/rand.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#define FALLBACK_KEY "aGFtcHVzIGlzIG1lZ2Egbg=="

int HTTP_Connect(HTTPClient* client, SSL_CTX* ctx, const char* host, const char* port) {
    struct addrinfo hints = {0},* res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host, port, &hints, &res);
    if (err != 0) {
        freeaddrinfo(res);
        return err;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1) {
        return -1;
    }

    err = connect(sock, (__CONST_SOCKADDR_ARG) {res->ai_addr}, res->ai_addrlen);
    if (err != 0) {
        freeaddrinfo(res);
        close(sock);
        return err;
    }

    freeaddrinfo(res);

    SSL* ssl = SSL_new(ctx);
    if (ssl == NULL) {
        ERR_print_errors_fp(stderr);
        close(sock);
        return -1;
    }

    if (SSL_set_fd(ssl, sock) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        return -1;
    }

    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        return -1;
    }

    client->sock = sock;
    client->ctx = ctx;
    client->ssl = ssl;
    strncpy(client->host, host, sizeof(client->host) - 1);
    strncpy(client->port, port, sizeof(client->port) - 1);
    client->connected = true;
    client->authorization[0] = '\0';

    return 0;
}

void HTTP_Disconnect(HTTPClient* client) {
    if (!client->connected) return;
    SSL_shutdown(client->ssl);
    SSL_free(client->ssl);
    close(client->sock);
    client->connected = false;
}

int HTTP_Reconnect(HTTPClient* client) {
    HTTP_Disconnect(client);
    return HTTP_Connect(client, client->ctx, client->host, client->port);
}

void HTTP_SetAuthorization(HTTPClient* client, const char* authorization) {
    strncpy(client->authorization, authorization, sizeof(client->authorization) - 1);
}

static HTTPResponse* HTTP_GetResponse(HTTPClient* client, Arena* arena) {
    size_t buf_size = 8192;
    size_t total = 0;
    char* buf = ArenaAlloc(GetTempArena(), buf_size);

    char* headers_end = NULL;
    while (!headers_end) {
        int r = SSL_read(client->ssl, buf + total, (int)(buf_size - total - 1));
        if (r <= 0) {
            free(buf);
            return NULL;
        }

        total += r;
        buf[total] = 0;
        headers_end = strstr(buf, "\r\n\r\n");
        if (!headers_end && total + 1024 > buf_size) {
            buf_size *= 2;
            buf = realloc(buf, buf_size);
        }
    }

    size_t header_len = (headers_end - buf) + 4;

    int status = -1;
    sscanf(buf, "HTTP/1.%*c %d", &status);

    size_t content_length = 0;
    char* cl = strcasestr(buf, "Content-Length:");
    if (cl != NULL) content_length = atoi(cl + 15);

    char* body = ArenaAlloc(arena, content_length + 1);
    size_t body_read = total - header_len;
    if (body_read > content_length) body_read = content_length;
    memcpy(body, buf + header_len, body_read);

    while (body_read < content_length) {
        int r = SSL_read(client->ssl, body + body_read, (int)(content_length - body_read));
        if (r <= 0) break;
        body_read += r;
    }

    body[body_read] = 0;

    HTTPResponse* res = ArenaAlloc(arena, sizeof(HTTPResponse));
    res->code = status;
    res->body = body;
    return res;
}

HTTPResponse* HTTP_Request(HTTPClient* client, Arena* arena, const char* method, const char* path, const char* body) {
    if (!client->connected && HTTP_Reconnect(client) != 0) return NULL;

    char auth[300] = "";
    if (client->authorization[0] != '\0') {
        snprintf(auth, sizeof(auth), "Authorization: Bot %s\r\n", client->authorization);
    }

    size_t body_len = body ? strlen(body) : 0;
    size_t req_len = strlen(method) + 1 + strlen(path) + 12
                   + strlen(client->host) + 64
                   + strlen(auth)
                   + 512
                   + body_len + 1;

    char* req = ArenaAlloc(GetTempArena(), req_len);

    int len = snprintf(req, req_len,
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: gambler/1.0\r\n"
        "Accept: application/json\r\n"
        "%s"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: keep-alive\r\n\r\n"
        "%s",
        method, path, client->host, auth, body_len, body != NULL ? body : "");

    if (SSL_write(client->ssl, req, len) != len) {
        if (HTTP_Reconnect(client) == 0)
            return HTTP_Request(client, arena, method, path, body);
        return NULL;
    }

    return HTTP_GetResponse(client, arena);
}

char* GenerateWebsocketKey(void) {
    unsigned char key[16];
    if (RAND_bytes(key, sizeof(key)) != 1) { // rng error somehow
        return CopyString(FALLBACK_KEY);
    }

    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, key, sizeof(key));
    BIO_flush(b64);

    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);

    char* b64text = HeapAlloc(bptr->length + 1);
    memcpy(b64text, bptr->data, bptr->length);
    b64text[bptr->length] = '\0';

    BIO_free_all(b64);
    return b64text;
}

int WS_Connect(WSClient* client, SSL_CTX* ctx, const char* host, const char* port, const char* path) {
    struct addrinfo hints = {0},* res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host, port, &hints, &res);
    if (err != 0) {
        freeaddrinfo(res);
        return err;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1) {
        return -1;
    }

    err = connect(sock, (__CONST_SOCKADDR_ARG) {res->ai_addr}, res->ai_addrlen);
    if (err != 0) {
        freeaddrinfo(res);
        close(sock);
        return err;
    }

    freeaddrinfo(res);

    SSL* ssl = SSL_new(ctx);
    if (ssl == NULL) {
        ERR_print_errors_fp(stderr);
        close(sock);
        return -1;
    }

    if (SSL_set_fd(ssl, sock) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        return -1;
    }

    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        return -1;
    }

    char* key = GenerateWebsocketKey();
    char req[512];
    snprintf(req, sizeof(req),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Key: %s\r\n"
             "Sec-WebSocket-Version: 13\r\n"
             "Sec-WebSocket-Protocol: json\r\n"
             "User-Agent: GamblerWebutils\r\n"
             "\r\n", path, host, key);

    HeapFree(key);

    int written = SSL_write(ssl, req, strlen(req));
    if (written <= 0 || written != strlen(req)) {
        ERR_print_errors_fp(stderr);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        return -1;
    }

    char resp[1024];
    int n = SSL_read(ssl, resp, sizeof(resp) - 1);
    if (n <= 0) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        return -1;
    }

    resp[n] = '\0';

    if (!strstr(resp, "101 Switching Protocols")) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        return -1;
    }

    client->sock = sock;
    client->ssl = ssl;

    return 0;
}

void WS_Disconnect(WSClient client, int _code) {
    if (_code != DONT_SEND_CODE) {
        uint16_t code = htons((uint16_t) _code);
        unsigned char close_frame[4];
        close_frame[0] = 0x88;
        close_frame[1] = 2;
        memcpy(close_frame + 2, &code, 2);
        SSL_write(client.ssl, close_frame, sizeof(close_frame));
    } else {
        unsigned char close_frame[2] = {0x88, 0x0};
        SSL_write(client.ssl, close_frame, sizeof(close_frame));
    }

    unsigned char buffer[512];
    SSL_read(client.ssl, buffer, sizeof(buffer));

    SSL_shutdown(client.ssl);
    SSL_free(client.ssl);
    close(client.sock);
}

int WS_SendText(WSClient* client, const char* text) {
    SSL* ssl = client->ssl;
    size_t len = strlen(text);
    unsigned char header[14];
    size_t header_len = 0;

    header[0] = 0x81; // FIN=1, opcode=0x1 (text)

    if (len <= 125) {
        header[1] = 0x80 | (unsigned char) len; // mask bit set
        header_len = 2;
    } else if (len <= 0xFFFF) {
        header[1] = 0x80 | 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        header_len = 4;
    } else {
        header[1] = 0x80 | 127;
        for (int i = 0; i < 8; i++)
            header[2 + i] = (len >> (56 - i * 8)) & 0xFF;
        header_len = 10;
    }

    unsigned char mask[4];
    for (int i = 0; i < 4; i++)
        mask[i] = rand() & 0xFF;

    memcpy(header + header_len, mask, 4);
    header_len += 4;

    unsigned char* masked = HeapAlloc(len);
    for (size_t i = 0; i < len; i++)
        masked[i] = text[i] ^ mask[i % 4];

    size_t total_sent = 0;
    while (total_sent < header_len) {
        int r = SSL_write(ssl, header + total_sent, (int) (header_len - total_sent));
        if (r <= 0) {
            HeapFree(masked);
            return -1;
        }
        total_sent += r;
    }

    total_sent = 0;
    while (total_sent < len) {
        int r = SSL_write(ssl, masked + total_sent, (int) (len - total_sent));
        if (r <= 0) {
            HeapFree(masked);
            return -1;
        }
        total_sent += r;
    }

    HeapFree(masked);
    return 0;
}

int WS_RecvText(WSClient* client, char** out_payload, Arena* arena) {
    SSL* ssl = client->ssl;
    unsigned char hdr[14];

    int n = SSL_read(ssl, hdr, 2);
    if (n != 2) return -1;

    unsigned char fin = hdr[0] & 0x80;
    unsigned char opcode = hdr[0] & 0x0F;
    int masked = (hdr[1] & 0x80) != 0;
    uint64_t payload_len = hdr[1] & 0x7F;

    if (payload_len == 126) {
        if (SSL_read(ssl, hdr + 2, 2) != 2) return -1;
        payload_len = ((uint64_t) hdr[2] << 8) | hdr[3];
    } else if (payload_len == 127) {
        if (SSL_read(ssl, hdr + 2, 8) != 8) return -1;
        payload_len = 0;
        for (int i = 0; i < 8; i++)
            payload_len = (payload_len << 8) | hdr[2 + i];
    }

    unsigned char mask[4] = {0};
    if (masked) {
        if (SSL_read(ssl, mask, 4) != 4) return -1;
    }

    if (opcode == 0x8) {
        client->last_close_code = 1000;
        if (payload_len >= 2) {
            unsigned char* tmp = HeapAlloc((size_t) payload_len);

            size_t read_total = 0;
            while (read_total < payload_len) {
                int r = SSL_read(ssl, tmp + read_total, payload_len - read_total);
                if (r <= 0) {
                    HeapFree(tmp);
                    return -1;
                }
                read_total += r;
            }
            client->last_close_code = (tmp[0] << 8) | tmp[1];
            HeapFree(tmp);
        } else if (payload_len > 0) {
            unsigned char dummy[1024];
            size_t remaining = payload_len;
            while (remaining > 0) {
                int to_read = remaining > sizeof(dummy) ? sizeof(dummy) : (int) remaining;
                int r = SSL_read(ssl, dummy, to_read);
                if (r <= 0) break;
                remaining -= r;
            }
        }
        return -1;
    }

    if (opcode != 0x1 && opcode != 0x0) return -1;

    unsigned char* payload = ArenaAlloc(arena, (size_t) payload_len + 1);

    size_t read_total = 0;
    while (read_total < payload_len) {
        int r = SSL_read(ssl, payload + read_total, (int) (payload_len - read_total));
        if (r <= 0) {
            return -1;
        }
        read_total += r;
    }

    if (masked) {
        for (uint64_t i = 0; i < payload_len; i++)
            payload[i] ^= mask[i % 4];
    }

    payload[payload_len] = '\0';
    *out_payload = (char*) payload;
    return (int) payload_len;
}

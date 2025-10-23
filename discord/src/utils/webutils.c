// Copyright 2025 JesusTouchMe

#define _GNU_SOURCE

#include "utils/webutils.h"

#include <openssl/rand.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>

#define FALLBACK_KEY "aGFtcHVzIGlzIG1lZ2Egbg=="

int SSL_read_all(SSL* ssl, char* buf, int max) {
    int total = 0;
    while (total < max) {
        int r = SSL_read(ssl, buf + total, max - total);
        if (r <= 0) break;
        total += r;
    }
    return total;
}

int HTTP_Connect(HTTPClient* client, SSL_CTX* ctx, const char* host, const char* port) {
    struct addrinfo hints = {0};
    struct addrinfo* res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host, port, &hints, &res);
    if (err != 0) {
        freeaddrinfo(res);
        return err;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1) {
        freeaddrinfo(res);
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
    char saved = client->authorization[0]; // XXX: this is a fuckass solution to a problem i created myself

    HTTP_Disconnect(client);
    int res = HTTP_Connect(client, client->ctx, client->host, client->port);

    if (res == 0) client->authorization[0] = saved;

    return res;
}

void HTTP_SetAuthorization(HTTPClient* client, const char* authorization) {
    strncpy(client->authorization, authorization, sizeof(client->authorization) - 1);
}

static int ParseStatusCode(const char* header) {
    const char* s = strstr(header, "HTTP/");
    if (s == NULL) return 0;
    s = strchr(s, ' ');
    if (s == NULL) return 0;
    return atoi(s + 1);
}

static size_t ParseContentLength(const char* header) {
    const char* cl = strcasestr(header, "Content-Length:");
    if (cl == NULL) return 0;
    return strtoul(cl + 15, NULL, 10);
}

static bool IsChunked(const char* header) {
    const char* te = strcasestr(header, "Transfer-Encoding:");
    return te != NULL && strcasestr(te, "chunked") != NULL;
}

static char* HTTP_ReadHeaders(HTTPClient* client) {
    size_t cap = 4096;
    size_t length = 0;
    char* buf = HeapAlloc(cap);

    while (true) {
        if (length + 1 >= cap) {
            cap *= 2;
            buf = HeapRealloc(buf, cap);
        }

        int r = SSL_read(client->ssl, buf + length, 1);
        if (r <= 0) {
            int err = SSL_get_error(client->ssl, r);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
            break;
        }

        length += r;
        if (length >= 4 && memcmp(buf + length - 4, "\r\n\r\n", 4) == 0) break;
    }

    buf[length] = 0;
    return buf;
}

static char* HTTP_ReadBody(HTTPClient* client, size_t length) {
    char* tmp = HeapAlloc(length + 1);
    size_t received = 0;

    while (received < length) {
        int r = SSL_read(client->ssl, tmp + received, (int)(length - received));
        if (r <= 0) {
            int err = SSL_get_error(client->ssl, r);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
            break;
        }
        received += r;
    }

    tmp[received] = 0;
    return tmp;
}

static char* HTTP_ReadChunkedBody(HTTPClient* client) {
    size_t total = 0;
    size_t cap = 4096;
    char* data = HeapAlloc(cap);

    while (true) {
        char line[64];
        size_t i = 0;

        while (i < sizeof(line) - 1) {
            int r = SSL_read(client->ssl, &line[i], 1);
            if (r <= 0) {
                int err = SSL_get_error(client->ssl, r);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                    continue;
                goto outtahere;
            }
            if (i >= 1 && line[i - 1] == '\r' && line[i] == '\n') break;
            i++;
        }

        line[i + 1] = '\0';
        size_t chunk_size = strtoul(line, NULL, 16);
        if (chunk_size == 0) break;

        if (total + chunk_size >= cap) {
            cap = (total + chunk_size) * 2;
            data = HeapRealloc(data, cap);
        }

        size_t read_bytes = 0;
        while (read_bytes < chunk_size) {
            int r = SSL_read(client->ssl, data + total + read_bytes,
                             (int)(chunk_size - read_bytes));
            if (r <= 0) {
                int err = SSL_get_error(client->ssl, r);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                    continue;
                goto outtahere;
            }
            read_bytes += r;
        }
        total += read_bytes;

        char skip[2];
        size_t skip_read = 0;
        while (skip_read < 2) {
            int r = SSL_read(client->ssl, skip + skip_read, 2 - skip_read);
            if (r <= 0) {
                int err = SSL_get_error(client->ssl, r);
                if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                    continue;
                goto outtahere;
            }
            skip_read += r;
        }
    }

outtahere:
    data[total] = '\0';
    return data;
}


static HTTPResponse* HTTP_GetResponse(HTTPClient* client, Arena* arena) {
    if (!client->connected && HTTP_Reconnect(client) != 0) return NULL;

    char* header = HTTP_ReadHeaders(client);
    //if (header == NULL) return NULL;

    int code = ParseStatusCode(header);
    char* tmp_body = NULL;

    if (IsChunked(header)) {
        tmp_body = HTTP_ReadChunkedBody(client);
    } else {
        size_t length = ParseContentLength(header);
        tmp_body = HTTP_ReadBody(client, length);
    }

    if (tmp_body == NULL) {
        HeapFree(header);
        return NULL;
    }

    HTTPResponse* res = ArenaAlloc(arena, sizeof(HTTPResponse));
    res->code = code;
    size_t body_length = strlen(tmp_body);
    res->body = ArenaAlloc(arena, body_length + 1);
    memcpy(res->body, tmp_body, body_length + 1);

    HeapFree(header);
    HeapFree(tmp_body);

    return res;
}

HTTPResponse* HTTP_Request(HTTPClient* client, Arena* arena, const char* method, const char* path, const char* body) {
    char auth[300] = "";
    if (client->authorization[0] != '\0') {
        snprintf(auth, sizeof(auth), "Authorization: %s\r\n", client->authorization);
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

    const int max_retries = 2;
    int attempt = 0;
    retry:

    if (!client->connected) {
        if (HTTP_Reconnect(client) != 0) return NULL;
    }

    int fd = SSL_get_fd(client->ssl);
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    if (poll(&pfd, 1, 0) > 0) {
        char tmp;
        if (recv(fd, &tmp, 1, MSG_PEEK | MSG_DONTWAIT) == 0) {
            client->connected = false;
            if (++attempt >= max_retries) return NULL;
            goto retry;
        }
    }

    int ret = SSL_write(client->ssl, req, len);
    if (ret != len) {
        int err = SSL_get_error(client->ssl, ret);
        if (err == SSL_ERROR_SYSCALL || err == SSL_ERROR_SSL || err == SSL_ERROR_ZERO_RETURN) {
            client->connected = false;
            if (++attempt >= max_retries) return NULL;
            goto retry;
        }
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

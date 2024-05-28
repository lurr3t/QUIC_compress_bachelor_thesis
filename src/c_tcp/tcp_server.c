#include <stdio.h>
#include <dirent.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <zlib.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>


#define MAX 4000// Increase buffer size to handle larger files
#define BUFFER_SIZE 1024 * 1024 * 80
#define PORT 8080
#define SA struct sockaddr
#define DIRECTORY "/home/lurr3t/exjobb/src/bin/"
#define DATETIME_SIZE 24
#define CHUNK 16384
char *end_time;

typedef struct {
    int compression;
    char *port;
    char *ip_address;
    char *cert_path;
    char *key_path;
} Args;

Args *args;

char *get_datetime_with_milliseconds();
char* decompress_buffer(unsigned char *inbuffer, size_t inbuffer_size, size_t *decompressed_size);

// Function to receive file count from client
size_t receive_file_count(SSL *ssl) {
    size_t count;
    int received = SSL_read(ssl, &count, sizeof(size_t));
    if (received < 0) {
        perror("Error when receiving file count:\n");
    }
    printf("%ld files in dir\n", count);
    return count;
}

// Function to receive file header from client
size_t receive_file_header(SSL *ssl) {
    size_t filesize;
    int received = SSL_read(ssl, &filesize, sizeof(size_t));
    if (received < 0) {
        perror("Error when receiving file size:\n");
    }
    printf("filesize is %ld bytes large\n", filesize);
    return filesize;
}

// Function to receive file data from client
void receive_file_data(SSL *ssl, char *filepath, size_t filesize) {
    // Create a buffer to hold the file data
    char *filedata = malloc(filesize);
    if (filedata == NULL) {
        fprintf(stderr, "Could not allocate memory for file data\n");
        return;
    }

    char buff[MAX];
    size_t bytes_received = 0;
    while (bytes_received < filesize) {
        int received = SSL_read(ssl, buff, sizeof(buff)); // Receive only the bytes that were sent
        if(received < 0) {
            int err = SSL_get_error(ssl, received);
            fprintf(stderr, "SSL_read failed with error %d\n", err);
            free(filedata);
            return;
        } else {
            // Write the received bytes to the buffer
            memcpy(filedata + bytes_received, buff, received);
            bytes_received += received;
        }
    }

    if (args->compression) {
        size_t decomp_size;
        filedata = decompress_buffer(filedata, filesize, &decomp_size);
        filesize = decomp_size;
        printf("decompressed to size %ld\n", filesize);
    }



    // Now write the buffer to the file
    FILE *file = fopen(filepath, "wb"); // Open the file in binary mode
    if (file != NULL) {
        fwrite(filedata, 1, filesize, file); // Write the buffer to the file
        fclose(file);
    } else {
        fprintf(stderr, "Could not open %s for writing\n", filepath);
    }

    // Free the buffer
    free(filedata);
}


// Main function to receive files from client
void func(SSL *ssl) {
    size_t count = receive_file_count(ssl);
    for (size_t i = 0; i < count; i++) {
        char filepath[256];
        sprintf(filepath, "%s/file%ld", DIRECTORY, i+1);
        size_t filesize = receive_file_header(ssl);
        receive_file_data(ssl, filepath, filesize);
    }

    //end time should be here
    end_time = get_datetime_with_milliseconds();
    // Send close message
    char close_buff[10] = "CLOSE";
    int n = SSL_write(ssl, close_buff, sizeof(close_buff));
    if (n < 0) {
        perror("Couldn't send close message: ");
    } else {
        printf("Closing connection\n");
    }
}


char *get_datetime_with_milliseconds() {
    // Get the current time
    char *datetime_string = malloc(DATETIME_SIZE);
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Convert time to struct tm for formatting
    time_t rawtime = tv.tv_sec;
    struct tm *tm_info = localtime(&rawtime);

    // Format the time as a string with milliseconds
    strftime(datetime_string, DATETIME_SIZE, "%Y-%m-%d %H:%M:%S", tm_info);

    // Append milliseconds to the datetime string
    sprintf(datetime_string + strlen(datetime_string), ".%03d", (int)tv.tv_usec / 1000);
    return datetime_string;
}



char* decompress_buffer(unsigned char *inbuffer, size_t inbuffer_size, size_t *decompressed_size) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (inflateInit(&strm) != Z_OK) {
        fprintf(stderr, "Could not initialize zlib\n");
        return NULL;
    }

    unsigned char out[CHUNK];
    int ret;

    strm.avail_in = inbuffer_size;
    strm.next_in = inbuffer;

    unsigned char *outbuffer = NULL;
    *decompressed_size = 0;

    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);
        switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&strm);
                free(outbuffer);
                return NULL;
        }
        unsigned have = CHUNK - strm.avail_out;
        outbuffer = realloc(outbuffer, *decompressed_size + have);
        memcpy(outbuffer + *decompressed_size, out, have);
        *decompressed_size += have;
    } while (strm.avail_out == 0);

    inflateEnd(&strm);

    return (ret == Z_STREAM_END ? outbuffer : NULL);
}





Args *parse_args(int argc, char *argv[]) {
    Args *args = malloc(sizeof(Args));
    args->compression = 0;
    args->port = NULL;
    args->ip_address = NULL;
    args->cert_path = NULL;
    args->key_path = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "zp:h:c:k:")) != -1) {
        switch (opt) {
            case 'z':
                args->compression = 1;
                break;
            case 'p':
                args->port = optarg;
                break;
            case 'h':
                args->ip_address = optarg;
                break;
            case 'c':
                args->cert_path = optarg;
                break;
            case 'k':
                args->key_path = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-z] [-p port] [-h ip_address] [-c cert_path] [-k key_path]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    return args;
}


// Driver function
int main(int argc, char **argv)
{
    args = parse_args(argc, argv);

    ///signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // Initialize the SSL library
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *meth = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(meth);

    // Load your server certificate and key
    SSL_CTX_use_certificate_file(ctx, args->cert_path, SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, args->key_path, SSL_FILETYPE_PEM);

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        //printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    if (strcmp(args->ip_address, "localhost") == 0) {
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        servaddr.sin_addr.s_addr = inet_addr(args->ip_address);
    }
    servaddr.sin_port = htons(atoi(args->port));

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(0);
    }
    else {
        printf("server accept the client...\n");
    }


    // Create an SSL structure for the connection
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, connfd);

    // Perform the SSL/TLS handshake with the client
    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    } else {
        // Function for receiving files
        func(ssl);
    }

    // Shutdown the connection
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(connfd);
    close(sockfd);


    SSL_CTX_free(ctx);
    printf("|%s", end_time);
    return 0;
}

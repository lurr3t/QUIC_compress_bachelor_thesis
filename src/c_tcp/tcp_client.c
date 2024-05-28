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
#include <getopt.h>


#define MAX 4000
#define PORT 8080
#define SA struct sockaddr
#define DIRECTORY "/home/lurr3t/exjobb/payloads/dynamic"
#define DATETIME_SIZE 24
#define CHUNK 16384
char *PATH_TO_COMPRESSED_FILES = "/home/lurr3t/exjobb/payloads/compressed";


typedef struct {
    int compression;
    char *port;
    char *ip_address;
    char *cert_path;
    char *key_path;
} Args;

Args *args;

char *get_datetime_with_milliseconds();
int compress_file(char *infilename, char *outdirectory, char *outfilename);


int count_files_in_directory() {
    char *directory;
    if (args->compression) {
        directory = PATH_TO_COMPRESSED_FILES;
    } else {
        directory = DIRECTORY;
    }
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(directory);
    if (dir == NULL) {
        perror("Unable to open directory");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // If the entry is a regular file then increment the count
            count++;
        }
    }

    closedir(dir);
    return count;
}


void send_file_count(SSL *ssl) {
    struct stat buf;
    size_t count = count_files_in_directory();
    printf("%ld files in dir\n", count);
    int sent = SSL_write(ssl, &count, sizeof(size_t));
    if (sent < 0) {
        perror("Error when sending file count:\n");
    }
}

// sends the size of the file
void send_file_header(SSL *ssl, char *filepath) {
    struct stat buf;
    stat(filepath, &buf);
    size_t filesize = buf.st_size;
    printf("filesize is %ld bytes large\n", filesize);

    int sent = SSL_write(ssl, &filesize, sizeof(size_t));
    printf("sent size buffer %d\n", sent);
    if (sent < 0) {
        perror("Error when sending file count:\n");
    }
}



// Function designed to send files from a directory to the server.
void func(SSL *ssl)
{
    char *directory;
    if (args->compression) {
        directory = PATH_TO_COMPRESSED_FILES;
    } else {
        directory = DIRECTORY;
    }

    send_file_count(ssl);
    DIR *d;
    struct dirent *dir;
    d = opendir(directory);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // Skip the special directory entries "." and ".."
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                continue;
            }

            char filepath[500];
            sprintf(filepath, "%s/%s", directory, dir->d_name);
            FILE *file = fopen(filepath, "rb"); // Open the file in binary mode
            printf("sending file %s\n", filepath);
            if (file != NULL) {

                send_file_header(ssl, filepath);

                char buff[MAX];
                int bytes_read;
                while ((bytes_read = fread(buff, 1, sizeof(buff), file)) > 0) {

                    int sent = SSL_write(ssl, buff, bytes_read); // Send only the bytes that were read
                    if(sent < 0) {
                        int err = SSL_get_error(ssl, sent);
                        fprintf(stderr, "SSL_write failed with error %d\n", err);
                        break;
                    } else {
                        //printf("sent %d\n", sent);
                    }
                }
            }
        }
        closedir(d);
    }


    // read close message
    char close_buff[10];
    int n = SSL_read(ssl, close_buff, sizeof(close_buff));
    if (n < 0) {
        perror("Couldn't read close message: ");
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



int compress_file(char *infilename, char *outdirectory, char *outfilename) {
    FILE *infile = fopen(infilename, "rb");
    if (!infile) {
        fprintf(stderr, "Could not open %s for reading\n", infilename);
        return -1;
    }

    char outpath[1024];
    snprintf(outpath, sizeof(outpath), "%s/%s", outdirectory, outfilename);
    FILE *outfile = fopen(outpath, "wb");
    if (!outfile) {
        fprintf(stderr, "Could not open %s for writing\n", outpath);
        return -1;
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
        fprintf(stderr, "Could not initialize zlib\n");
        return -1;
    }

    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    int flush;

    do {
        strm.avail_in = fread(in, 1, CHUNK, infile);
        if (ferror(infile)) {
            deflateEnd(&strm);
            return -1;
        }
        flush = feof(infile) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            deflate(&strm, flush);
            unsigned have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, outfile) != have || ferror(outfile)) {
                deflateEnd(&strm);
                return -1;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);

    } while (flush != Z_FINISH);

    deflateEnd(&strm);
    fclose(infile);
    fclose(outfile);

    return 0;
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


int main(int argc, char **argv)
{
    args = parse_args(argc, argv);

    char *start_time = get_datetime_with_milliseconds();

    // Compress files if flag is set
    if (args->compression) {
        DIR *d;
        struct dirent *dir;
        d = opendir(DIRECTORY);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                    continue;
                }
                printf("compressing %s\n", dir->d_name);
                char inpath[1024];
                snprintf(inpath, sizeof(inpath), "%s/%s", DIRECTORY, dir->d_name);
                compress_file(inpath, PATH_TO_COMPRESSED_FILES, dir->d_name);
            }
        }
        closedir(d);
    }



    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;


    // Initialize the SSL library
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *meth = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(meth);

    // Load your client certificate and key
    SSL_CTX_use_certificate_file(ctx, args->cert_path, SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, args->key_path, SSL_FILETYPE_PEM);

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    if (strcmp(args->ip_address, "localhost") == 0) {
        servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        servaddr.sin_addr.s_addr = inet_addr(args->ip_address);
    }

    servaddr.sin_port = htons(atoi(args->port));



    // connect the client socket to server socket. Tries to connect multiple times
    while (1) {
        if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
            printf("connection with the server failed...\n");
            //exit(0);
        } else {
            printf("connected to the server..\n");
            break;
        }
    }

    // Create an SSL structure for the connection
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    // Perform the SSL/TLS handshake with the server
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    } else {
        // function for sending files
        func(ssl);
    }


    // Shutdown the connection
    SSL_shutdown(ssl);
    SSL_free(ssl);

    // close the socket
    close(sockfd);
    SSL_CTX_free(ctx);

    printf("|%s", start_time);

    free(args);

    return 0;
}

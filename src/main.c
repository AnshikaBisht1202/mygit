#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <zlib.h>
#include <openssl/sha.h>

void init()
{
    fprintf(stderr, "Logs from your program will appear here!\n");

        if (mkdir(".git", 0755) == -1 || 
            mkdir(".git/objects", 0755) == -1 || 
            mkdir(".git/refs", 0755) == -1) {
            fprintf(stderr, "Failed to create directories: %s\n", strerror(errno));
            exit(1);
        }
         
        FILE *headFile = fopen(".git/HEAD", "w");
         if (headFile == NULL) {
             fprintf(stderr, "Failed to create .git/HEAD file: %s\n", strerror(errno));
             exit(2);
        }
        fprintf(headFile, "ref: refs/heads/main\n");
        fclose(headFile);
        
        printf("Initialized git directory\n");
    }


void cat_file(char *flag, char *hash)
{
    char file_path[64];
    sprintf(file_path, ".git/objects/%c%c/%s", hash[0], hash[1], hash + 2);

    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
        exit(40);
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    Bytef *compressed_data = malloc(file_size);
    if (compressed_data == NULL) {
        fprintf(stderr, "Failed to allocate memory: %s\n", strerror(errno));
        fclose(fp);
        exit(60);
    }
    fread(compressed_data, 1, file_size, fp);
    fclose(fp);

    uLongf decompressed_size = 4096;
    Bytef *decompressed_data = malloc(decompressed_size);
    if (decompressed_data == NULL) {
        fprintf(stderr, "Failed to allocate memory: %s\n", strerror(errno));
        free(compressed_data);
        exit(70);
    }
    int res = uncompress(decompressed_data, &decompressed_size, compressed_data, file_size);
    if (res != Z_OK) {
        fprintf(stderr, "Failed to decompress data: %d\n", res);
        free(compressed_data);
        free(decompressed_data);
        exit(80);
    }
    if (res == Z_BUF_ERROR) {
        fprintf(stderr, "Buffer too small for decompression\n");
        free(compressed_data);
        free(decompressed_data);
        exit(90);
    }

    char *ptr = memchr(decompressed_data, '\0', decompressed_size)+1;
    if (ptr == NULL) {
        fprintf(stderr, "Failed to find null terminator in decompressed data\n");
        free(compressed_data);
        free(decompressed_data);
        exit(100);
    }
    size_t content_size = decompressed_size - (ptr - (char *)decompressed_data);

    fwrite(ptr, 1, content_size, stdout);
}

void hash_object(char *flag, char *file_path)
{
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
        exit(110);
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    char *file_content = malloc(file_size);
    if (file_content == NULL) {
        fprintf(stderr, "Failed to allocate memory: %s\n", strerror(errno));
        fclose(fp);
        exit(120);
    }
    fread(file_content, 1, file_size, fp);
    fclose(fp);

    char header[64];

    int header_len = snprintf(header, sizeof(header), "blob %ld", file_size);

    int full_len = header_len + 1 + file_size;

    char *buffer = malloc(full_len);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory: %s\n", strerror(errno));
        free(file_content);
        exit(130);
    }

    memcpy(buffer,header, header_len);
    buffer[header_len] = '\0';
    memcpy(buffer + 1 + header_len, file_content, file_size);

    unsigned char sha_hash[SHA_DIGEST_LENGTH];

    SHA1(buffer, full_len, sha_hash);

    char sha_hex[41];

    for (int i =0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(sha_hex + i * 2, "%02x", sha_hash[i]);
    }
    sha_hex[40] = '\0';
}


int main(int argc, char *argv[]) {
   
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 2) {
        fprintf(stderr, "Usage: ./your_program.sh <command> [<args>]\n");
        return 1;
    }
    
    const char *command = argv[1];
    
    if (strcmp(command, "init") == 0) {
        init();
    }
    else if (strcmp(command, "cat-file") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: ./your_program.sh cat-file <flag> <hash>\n");
            return 1;
        }
        cat_file(argv[2], argv[3]);
    }
    else if (strcmp(command, "hash-object") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: ./your_program.sh hash-object <file>\n");
            return 1;
        }
        hash_object(argv[2], argv[3]);
    }
    else {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }
    
    return 0;
}

#include <toolkit.h>
#include "cmd_handle.h"

#include <iostream>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <openssl/sha.h> 
#include <openssl/ssl.h>
#include <openssl/err.h>


int main() {
    // 1. INITIALIZE THE OPENSSL LIBRARY CORES
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    
    // 2. LOAD CERTIFICATE FILES
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Error loading certificates! Run the openssl command to create them." << std::endl;
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(8080);
    
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);
    
    while (true) {
        std::cout << "Waiting for browser connection..." << std::endl;
        int windows_client = accept(server_fd, nullptr, nullptr);
        
        // 3. CREATE SSL INSTANCE FOR THIS CLIENT CONNECTION
        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, windows_client);
        
        // Perform the underlying TLS cryptographic handshake
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(windows_client);
            continue;
        }
        
        char buffer[4096];
        std::memset(buffer, 0, sizeof(buffer));
        
        // 4. SECURE READ (Switched from raw read to SSL_read)
        int bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        
        if (bytes_read > 0) {
            std::string request(buffer);
            std::cout << "--- Got Browser Request ---" << std::endl;
            
            std::string search_target = "Sec-WebSocket-Key: ";
            size_t key_pos = request.find(search_target);
            
            if (key_pos != std::string::npos) {
                size_t start = key_pos + search_target.length();
                size_t end = request.find("\r\n", start);
                std::string browser_key = request.substr(start, end - start);
                
                std::cout << "Extracted Browser Key: " << browser_key << std::endl;
                
                std::string magic_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                std::string combined = browser_key + magic_guid;
                
                unsigned char sha1_result[20];
                SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), sha1_result);
                
                std::string accept_key = base64_encode(sha1_result, 20);
                std::cout << "Generated Accept Key: " << accept_key << std::endl;
                
                std::string response = 
                    "HTTP/1.1 101 Switching Protocols\r\n"
                    "Upgrade: websocket\r\n"
                    "Connection: Upgrade\r\n"
                    "Sec-WebSocket-Accept: " + accept_key + "\r\n\r\n";
                    
                // 5. SECURE WRITE (Switched from send to SSL_write)
                SSL_write(ssl, response.c_str(), response.length());
                std::cout << "Handshake Sent! Connection upgraded to WebSocket." << std::endl;

                while (true) {
                    unsigned char header[2];
                    int r = SSL_read(ssl, header, 2);
                    if (r <= 0) {
                        std::cout << "Client disconnected." << std::endl;
                        break;
                    }

                    int opcode = header[0] & 0x0F;
                    if (opcode == 0x8) {
                        std::cout << "Client sent close frame." << std::endl;
                        break;
                    }

                    bool is_masked = (header[1] & 0x80) != 0;
                    uint64_t payload_len = header[1] & 0x7F;

                    if (payload_len == 126) {
                        unsigned char ext_len[2];
                        SSL_read(ssl, ext_len, 2);
                        payload_len = (ext_len[0] << 8) | ext_len[1];
                    } else if (payload_len == 127) {
                        unsigned char ext_len[8];
                        SSL_read(ssl, ext_len, 8);
                        payload_len = 0;
                        for (int i = 0; i < 8; i++) {
                            payload_len = (payload_len << 8) | ext_len[i];
                        }
                    }

                    unsigned char masking_key[4] = {0};
                    if (is_masked) {
                        SSL_read(ssl, masking_key, 4);
                    }

                    std::string payload_bytes(payload_len, '\0');
                    int bytes_received = 0;
                    while (bytes_received < payload_len) {
                        int n = SSL_read(ssl, &payload_bytes[bytes_received], payload_len - bytes_received);
                        if (n <= 0) break;
                        bytes_received += n;
                    }

                    if (is_masked) {
                        for (size_t i = 0; i < payload_len; i++) {
                            payload_bytes[i] ^= masking_key[i % 4];
                        }
                    }

                    if (opcode == 0x1) {
                        std::cout << "Decoded Message from Website: " << payload_bytes << std::endl;
                    }

                    CMD::handle(payload_bytes);
                }

                SSL_shutdown(ssl);
                SSL_free(ssl);
                close(windows_client);
                continue; // Skip the double-close statement at the bottom
            }
        }
        
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(windows_client);
    }
    
    SSL_CTX_free(ctx);
    close(server_fd);
    return 0;
}

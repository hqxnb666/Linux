/*******************************************************************************
 * Simple Image Client
 *
 * Description:
 *     A simple client program that connects to the image processing server,
 *     registers an image, performs some operations, and retrieves the image.
 *
 * Usage:
 *     ./client <server_ip> <port_number> <image_file>
 *
 * Parameters:
 *     server_ip    - The IP address of the server.
 *     port_number  - The port number the server is listening on.
 *     image_file   - The path to the image file to upload.
 *
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "common.h"
#include "imglib.h"
#include "timelib.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Usage: %s <server_ip> <port_number> <image_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *server_ip = argv[1];
    int port_number = atoi(argv[2]);
    char *image_file = argv[3];

    int sockfd;
    struct sockaddr_in server_addr;
    struct request req;
    struct response resp;

    /* 创建套接字 */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("ERROR: 无法创建套接字");
        return EXIT_FAILURE;
    }

    /* 设置服务器地址 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("ERROR: 无效的服务器 IP 地址");
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* 连接到服务器 */
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR: 连接服务器失败");
        close(sockfd);
        return EXIT_FAILURE;
    }
    printf("INFO: 已连接到服务器 %s:%d\n", server_ip, port_number);

    /* 加载图像 */
    struct image *img = loadBMP(image_file);
    if (!img) {
        printf("ERROR: 无法加载图像 %s\n", image_file);
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* 注册图像 */
    memset(&req, 0, sizeof(req));
    req.req_id = 1;
    req.req_timestamp = get_current_timespec();
    req.img_op = IMG_REGISTER;
    req.overwrite = 0;
    req.img_id = 0;

    if (send(sockfd, &req, sizeof(req), 0) < 0) {
        perror("ERROR: 发送注册请求失败");
        deleteImage(img);
        close(sockfd);
        return EXIT_FAILURE;
    }
    sendImage(img, sockfd);

    /* 接收服务器响应 */
    if (recv(sockfd, &resp, sizeof(resp), 0) <= 0) {
        perror("ERROR: 接收响应失败");
        deleteImage(img);
        close(sockfd);
        return EXIT_FAILURE;
    }
    if (resp.ack != RESP_COMPLETED) {
        printf("ERROR: 图像注册被拒绝\n");
        deleteImage(img);
        close(sockfd);
        return EXIT_FAILURE;
    }
    uint64_t img_id = resp.img_id;
    printf("INFO: 图像已注册，分配的 img_id = %ld\n", img_id);

    /* 请求图像处理操作，例如旋转 */
    memset(&req, 0, sizeof(req));
    req.req_id = 2;
    req.req_timestamp = get_current_timespec();
    req.img_op = IMG_ROT90CLKW;
    req.overwrite = 0;
    req.img_id = img_id;

    if (send(sockfd, &req, sizeof(req), 0) < 0) {
        perror("ERROR: 发送图像处理请求失败");
        deleteImage(img);
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* 接收服务器响应 */
    if (recv(sockfd, &resp, sizeof(resp), 0) <= 0) {
        perror("ERROR: 接收响应失败");
        deleteImage(img);
        close(sockfd);
        return EXIT_FAILURE;
    }
    if (resp.ack != RESP_COMPLETED) {
        printf("ERROR: 图像处理请求被拒绝\n");
        deleteImage(img);
        close(sockfd);
        return EXIT_FAILURE;
    }
    uint64_t processed_img_id = resp.img_id;
    printf("INFO: 图像处理完成，新的 img_id = %ld\n", processed_img_id);

    /* 检索处理后的图像 */
    memset(&req, 0, sizeof(req));
    req.req_id = 3;
    req.req_timestamp = get_current_timespec();
    req.img_op = IMG_RETRIEVE;
    req.overwrite = 0;
    req.img_id = processed_img_id;

    if (send(sockfd, &req, sizeof(req), 0) < 0) {
        perror("ERROR: 发送检索请求失败");
        deleteImage(img);
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* 接收服务器响应 */
    if (recv(sockfd, &resp, sizeof(resp), 0) <= 0) {
        perror("ERROR: 接收响应失败");
        deleteImage(img);
        close(sockfd);
        return EXIT_FAILURE;
    }
    if (resp.ack != RESP_COMPLETED) {
        printf("ERROR: 图像检索被拒绝\n");
        deleteImage(img);
        close(sockfd);
        return EXIT_FAILURE;
    }

    /* 接收图像 */
    struct image *retrieved_img = recvImage(sockfd);
    if (!retrieved_img) {
        printf("ERROR: 无法接收图像\n");
        deleteImage(img);
        close(sockfd);
        return EXIT_FAILURE;
    }
    printf("INFO: 成功检索到图像，保存为 output.bmp\n");

    /* 保存接收到的图像 */
    if (!saveBMP("output.bmp", retrieved_img)) {
        printf("ERROR: 无法保存图像\n");
    }

    /* 关闭连接，释放资源 */
    deleteImage(img);
    deleteImage(retrieved_img);
    close(sockfd);
    return EXIT_SUCCESS;
}


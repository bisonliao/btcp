#include "btcp_api.h"
#include <poll.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <glib.h>
#include <unistd.h>
/*
 * 正如文件名字所说，这是一个 tcp client的用户程序 demo
 */

static struct btcp_tcpconn_handler handler;
void handle_sigint(int signum) {
    printf("Caught SIGINT (Ctrl+C), signum = %d\n", signum);
    btcp_destroy_tcpconn(&handler, false);
    exit(0);
}
int main(int argc, char** argv)
{
    

    signal(SIGINT, handle_sigint);
    
    if (btcp_tcpcli_connect("192.168.0.11", 8080, &handler))
    {
        g_warning("btcp_tcpcli_connect failed! %d", btcp_errno);
        return -1;
    }
    g_info("in main(), peer ip:%s, mss:%d, peer_port:%d\n", handler.peer_ip, handler.mss, handler.peer_port);
    
    btcp_tcpcli_new_loop_thread(&handler); // 启动tcp引擎的工作线程
    
    uint64_t total_write = 0; // 统计用户层发送了多少字节
    uint64_t total_read = 0;
    while (1)
    {
        if (handler.status != ESTABLISHED)
        {
            usleep(1000);
            continue;
        }

        

        // 准备 26个英文小写字母
        char buf[1024];
        ssize_t sz = 26;
        
        for (int i = 0; i < sz; ++i)
        {
            buf[i] = 'a'+i;
        }
        int offset = 0;
       
        if (total_write < 1000)
        {
            // 不断的通过tcp连接发送给服务器
            while (1)
            {
                int iret = write(handler.user_socket_pair[0], buf + offset, sz - offset);
                if (iret < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        usleep(100);
                        continue;
                    }
                    else
                    {
                        perror("write");
                        return -1;
                    }
                }
                else
                {
                    total_write += iret;
                    offset += iret;
                    g_message(">>>>>>>>>total write=%llu", total_write);
                    if (offset == sz)
                    {
                        break;
                    }
                }
            }
        }
        struct sockaddr_in client_addr;
        int received = btcp_is_readable(handler.user_socket_pair[0], 100, buf, sizeof(buf), &client_addr);
        if (received > 0)
        {
            buf[received] = 0;
            printf("received:%s\n", buf);
            total_read += received;
            g_message(">>>>>>>>>total read=%llu", total_read);
        }

        if (total_write > 1000 && total_write == total_read)
        {
            break;
        }
        
        usleep(100000);

        
    }
    g_message("client close the conn");
    close(handler.user_socket_pair[0]);
    int secs;
    for (secs = 0; secs < 3; ++secs)
    {
        usleep(1000000);
        g_message("status=%d", handler.status);
    }
    btcp_destroy_tcpconn(&handler, false);
   
    return 0;
}


#define __MLWIP_HTTP_C_
#include "mlwip_http.h"
#include "page.h"

#include <string.h>
#include <sys/socket.h>
#include "storage.h"
#include "wifi_event.h"


#define DBG_TAG "http"
#include "log.h"


int get_http_command(char *http_msg, char *command)
{
    char *p_end = http_msg;
    char *p_start = http_msg;
    while (*p_start) // GET /
    {
        if (*p_start == '/')
        {
            break;
        }
        p_start++;
    }
    if (p_start == NULL)
    {
        return -1;
    }

    p_end = strchr(http_msg, '\n');
    while (p_end != p_start)
    {
        if (*p_end == ' ')
        {
            break;
        }
        p_end--;
    }
    int len = p_end - p_start;
    strncpy(command, p_start, p_end - p_start);
    return len;
}


void http_server_thread(void *msg);
void mhttp_server_init()
{
    //常用变量
    int ss, sc;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int snd_size = 0; /* 发送缓冲区大小 */
    socklen_t optlen; /* 选项值长度 */
    // int optlen;
    int err;
    socklen_t addrlen;
    // int addrlen;

    //建立套接字
    ss = socket(AF_INET, SOCK_STREAM, 0);

    if (ss < 0)
    {
        printf("socket error\n");
    }

    /*设置服务器地址*/
    bzero(&server_addr, sizeof(server_addr));

    /*清零*/
    server_addr.sin_family = AF_INET;
    /*协议族*/
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /*本地地址*/
    server_addr.sin_port = htons(80);
    /*服务器端口*/

    /*绑定地址结构到套接字描述符*/
    err = bind(ss, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (err < 0)
    {
        printf("bind error\n");
        return -1;
    }

    /*设置侦听*/
    err = listen(ss, 7);

    if (err < 0)
    {
        printf("listen error\n");
        return -1;
    }

    addrlen = sizeof(struct sockaddr_in);
    MYPARM parm11;
    while (1)
    {
        LOG_I("accept start\r\n");
        sc = accept(ss, (struct sockaddr *)&client_addr, &addrlen);
        if ((sc < 0))
        {
            LOG_I("accept fail sc is:%d\r\n", sc);
            if (sc > 0)
            {
                close(sc);
            }
            continue;
        }

        parm11.sc = sc;
        parm11.buf = NULL;

        http_server_thread(&parm11);
        vTaskDelay(1);
    }
}

void http_server_thread(void *msg)
{
    // LOG_I("http_server_thread\r\n");
    MYPARM *parm11;
    parm11 = (MYPARM *)msg;

    int sc;
    char readbuffer[1024];
    int size = 0;
    char command[1024];
    char head_buf[1000];
    memset(command, 0, sizeof(command));
    memset(head_buf, 0, sizeof(head_buf));
    sc = parm11->sc;

    memset(readbuffer, 0, sizeof(readbuffer));


    while (1)
    {
        // LOG_I("read stop\r\n");
        size = read(sc, readbuffer, 1024);
        // int rc = recv(sc, readbuffer, sizeof(readbuffer), 0);
        LOG_I("read len:%d\r\n", size);
        LOG_I("get:%s\r\n", readbuffer);

        if (size <= 0)
        {
            LOG_I("size <= 0\r\n");
            break;
        }
        int len = get_http_command(readbuffer, command); //得到http 请求中 GET后面的字符串
        LOG_I("get:%s len:%d\r\n", command, len);

        if (strcmp(command, "/") == 0)
        {
            LOG_I("command1\r\n");
            
            sprintf(head_buf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html;charset=UTF-8\r\n\r\n", sizeof(html_page));

            // head_buf
            // strlen(index_ov2640_html)
            // 返回html页面
            int ret = write(sc, head_buf, strlen(head_buf));
            if (ret == -1)
            {
                LOG_I("send failed");
                close(sc);
                return NULL;
            }
            ret = write(sc, html_page, sizeof(html_page));
            if (ret < 0)
            {
                LOG_I("text write failed");
            }
            close(sc);
            break;
        } 
        else if (strstr(command, "set"))
        {
            LOG_I("set\r\n");
            
            // 获取POST提交过来的数据，拿到ssid部分
            char* wifiCfg = strstr(readbuffer, "ssid");

            LOG_I("wifiCfg: %s \r\n", wifiCfg);

            sprintf(head_buf, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 433\r\nAccess-Control-Allow-Origin: *\r\n\r\n");

            int ret = write(sc, head_buf, strlen(head_buf));
            if (ret == -1)
            {
                LOG_I("send failed");
                close(sc);
                return NULL;
            }

            // 将post参数切开，分别拿到 ssid 和 pwd
            char* ssid = strtok(wifiCfg, "&");
            char* pwd =  strtok(NULL, "&");
            char* server =  strtok(NULL, "&");
            char* name =  strtok(NULL, "&");
            char* pass =  strtok(NULL, "&");

            // 将ssid参数切开，分别拿到 key 和 value
            char* ssidKey = strtok(ssid, "=");
            char* ssidValue = strtok(NULL, "=");
            // 将pwd参数切开，分别拿到 key 和 value
            char* pwdKey = strtok(pwd, "=");
            char* pwdValue = strtok(NULL, "=");
            // 
            // 
            char* serverKey = strtok(server, "=");
            char* serverValue = strtok(NULL, "=");
            // 
            char* nameKey = strtok(name, "=");
            char* nameValue = strtok(NULL, "=");
            // 
            char* passKey = strtok(pass, "=");
            char* passValue = strtok(NULL, "=");
            
            LOG_I("OK ssid:%s, pwd:%s, server:%s, name:%s, pass:%s  \r\n", ssidValue, pwdValue, serverValue, nameValue, passValue);
        

            char data_buf[512];
            // char *data_buf_ptr,c_for='}';
            memset(data_buf,'\n',sizeof(data_buf));

            sprintf(data_buf, "{\'ssid\':\'%s\',\'password\':\'%s\'}",ssidValue,pwdValue);
            LOG_I("OK data_buf:%s  \r\n", data_buf);

            if (ssidValue == NULL || pwdValue == NULL)
            {
                continue;
            }
            if (strlen(ssidValue) <= 0 || strlen(pwdValue) <= 0)
            {
                continue;
            }
            
            // 将获取到的数据返回
            ret = write(sc, data_buf, sizeof(data_buf));
            if (ret < 0)
            {
                LOG_I("text write failed");
            }
            close(sc);

            // ============================================
            
            // 待实现 将 wifi 信息写入 存储
            flash_erase_set(SSID_KEY, ssidValue);
            flash_erase_set(PASS_KEY, pwdValue);
            // try_times
            flash_erase_set(TRY_TIMES,"3");
            // 
            flash_erase_set(MQTT_SERVER,serverValue);
            flash_erase_set(MQTT_NAME,nameValue);
            flash_erase_set(MQTT_PASS,passValue);


            // 开启 sta 模式，连接WIFI
            // 重启设备
            LOG_W("system 1.5s reset ");
            vTaskDelay(1500/portTICK_PERIOD_MS);
            GLB_SW_System_Reset();

            // ============================================

            break;
        }
        else
        {
            close(sc);
        }
        // write(sc,readbuffer,size);

    }
    //关中断
    // free(parm11->buf);
    // vTaskDelete(NULL);
    //开中断
}


// -------------------------------------none

static const unsigned char html_page[] = R"(
    <html>
        <body>
            欢迎来到Ai-HOMO配置界面!
            <form method="post" action="/set">
            <input name="ssid" type="text" placeholder="WiFi名称" />
            <br/>
            <input name="pwd" type="text" placeholder="WiFi密码" />
            <br/>
            <input type="text" name="server" value="192.168.6.1">
            <br/>
            <input type="text" name="name" value="lzb">
            <br/>
            <input type="text" name="pass" value="lzb">
            <br/>

            <input type="submit" value="提交"/>
            </form>
        </body>
    </html>
)";
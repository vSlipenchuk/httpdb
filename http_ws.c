/*

  web Socket event notifier

*/

#include "httpSrv.h"
#include "vss.h"

int ws_cnt = 0; // web_socket counter...

#include <openssl/sha.h>
// need - -lcrypto -lssl
int calcWSdigest(char *resp,unsigned char *key) { //
unsigned char obuf[20];
unsigned char ibuf[256];
if (strlen(key)>80) return 0; // too big?
ibuf[0]=0;  strcpy(ibuf,key); strcat(ibuf,
                                      //258EAFA5-E914-47DA-95CA-C5AB0DC85B11
                                     "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
SHA1(ibuf, strlen(ibuf), obuf);
// andbase 64
encode_base64(resp,obuf,20);
return 1; // Ok - anyway
}

/*
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
Sec-WebSocket-Protocol: chat
*/


int ws_mkstr(char *d,char *s, int len) {
if (len<0) len=strlen(s);
if (!d) return len+2+4+1;
 //d[0]=0x81; d1=0x80
return len+2+4+1;
}

int do_echo_sock(int echo_sock) {
//char b[3]={0x80,0x1,'H'};
//char b[7]={0x00,'H','e','l','l','o',0xFF};
//
sleep(1);
//char b[12]={0x81, 0x86,  0x11,0x22,0x33,0x44,    0x59,0x47,0x5F,0x28,0x7E, 0x22}; // masked ?
char b[12]={0x81, 0x05,  'h' ,'e' ,'l' ,'l' ,'o'};
//sleep(3);
sock_write(echo_sock,b,sizeof(b)); // send now


while(1) {
   char buf[256];
   int len = sock_read(echo_sock,buf,sizeof(buf));
   printf("READ %d bytes from %d\n",len, echo_sock);
   if (len==-1) break; // closed, read error
   if (len>0) {

  // sock_write(echo_sock,buf,len);
    }
    msleep(1000);
}
}



int SocketSendWebSocketAccept(Socket *sock, vssHttp *req,char *key)
{char buf[1024];
vss reqID = {0,0};
if (req && req->reqID.len>0) reqID=req->reqID;
//if (len<0) len = strlen(data);
sprintf(buf,
        "HTTP/1.1 101\r\n" //" Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-Websocket-Accept: %s\r\n"
   //     "Sec-WebSocket-Protocol: chat\r\n"
        //"WebSocket-Origin: http://localhost\r\n"
        //"WebSocket-Location: ws://localhost/event\r\n"
        "\r\n",key);
/*
sprintf(buf,
           "HTTP/1.1 101 Switching Protocols\r\n"
	"Upgrade: websocket\r\n"
	"Connection: Upgrade\r\n"
	"Sec-WebSocket-Accept: %s\r\n"
	"Sec-WebSocket-Protocol: chat\r\n"
	"\r\n"
        ,key
        );

*/

strCat(&sock->out,buf,-1); // Add a header
//strCat(&sock->out,data,len); // Push it & Forget???
//printf("TOSEND:%s\n",sock->out);
sock->state = sockSend;

/* mark me as web socket */
ws_cnt++;  sprintf(sock->name,"WS:%d",ws_cnt);
//thread_create( do_echo_sock, (void*)sock->sock);
//sock->checkPacket = checkPacketEcho; //onData = onData;
return 1;
}




int extractMimeHeader(vss *src,char *name, char *buf, int size) {
vss val;
memset(buf,0,size);
if (!vssFindMimeHeader(src,name,&val)) { *name=0; return 0;} // empty
size--; if (val.len>=size) val.len=size;
memcpy(buf,val.data,val.len);
return 1; // found
}

int wsSendStr(Socket *sock,char *msg) {
int l = strlen(msg);
if (l>126) l=126;
char b[2] = { 0x81, l }; // no zero term?
SocketSend(sock,b,2);
SocketSend(sock,msg,l);
return 0;
}

int wsBroadcast(SocketPool *sp, char *msg) {

//CLOG(srv,7,"SocketPool - run %d sockets",arrLength(sp->sock));
//printf("SocketPool - run %d sockets",arrLength(sp->sock));
//rep = srv->report && (now!=sp->reported);sp->reported=now;
int i,cnt=0;
for(i=0;i<arrLength(sp->sock);i++) {
    Socket *s = sp->sock[i];
    //printf("NAME[%d]:%s\n",i,s->name);
    if ( memcmp(s->name,"WS",2)==0) {
            wsSendStr(s,msg);
            cnt++;
         }
     }
printf("Broadcasted %d out-of %d LEN sockets\n",cnt,i);
return cnt;
}

int onWebSocket(Socket *sock, vssHttp *req, SocketMap *map) { // Генерация статистики по серверу
char buf[1024];
httpSrv *srv = (void*)sock->pool;
strSetLength(&srv->buf,0); // ClearResulted
vss r=req->B;
strCat(&srv->buf,r.data,r.len); char *sql = srv->buf;
printf("WebSocket url:<%*.*s>\n",VSS(req->U));
char key[256],res[256];
extractMimeHeader(&req->H,"Sec-Websocket-Key",key,sizeof(key));

//strcpy(key,"dGhlIHNhbXBsZSBub25jZQ=="); // tmp
  calcWSdigest(res,key);
printf("Key: <%s> Resp: <%s>\n",key,res);

  //http.sec_websocket_version -- 13
  //http.sec_websocket_key

SocketSendWebSocketAccept(sock,req,res);
//SocketSendDataNow(sock,0,0);
//sock->sock=0;//  get

wsSendStr(sock,"Welcome newWebSocket");
return 1;
}

int vssGetVar2Buffer(vss arg,char *name,char *buf, int size) ; // to do - url_decode_param?

int onBroadcast(Socket *sock, vssHttp *req, SocketMap *map) { // Генерация статистики по серверу
char msg[256];
httpSrv *srv = (void*)sock->pool;
strSetLength(&srv->buf,0); // ClearResulted
vss r=req->B;
vssGetVar2Buffer(req->args,"msg",msg,sizeof(msg));
wsBroadcast((void*)sock->pool,msg); // all
SocketPrintHttp(sock,req,"+OK");
return 1;
}






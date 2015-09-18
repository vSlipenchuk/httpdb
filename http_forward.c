/*
  http forwarder
*/

#include "vos.h"

int _httpSyncExchange(char *host, char *sdata, int slen, int ta, char **out) {
int sock;
sock = sock_connect(host,80); if (sock<=0) return -1; // connect error
if (ta<=0) ta = 3600; // one hour
while(slen>0) {
   int l = send(sock,sdata,slen,0);
   if (l<0) {sock_close(sock); return -2; } // sent error
   slen-=l; sdata+=l;
   }
// ok - all done - need to read
int begin = strLength(*out);
while(ta>0) {
   char buf[1024];
   if (!sock_readable(sock)) {  sleep(1);   ta--;   continue; }
   int l = recv(sock,buf,sizeof(buf),0); // no timeout?
   if (l<=0) break;
   strCat(out,buf,l);
   if (httpReady((*out)+begin)) { sock_close(sock); return 2; } // ok
   }
sock_close(sock);
return 1; // OK
}

int httpSyncGet(char *host,char *url,int ta,char **out) { // cat result to out
char buf[1024];
sprintf(buf,"GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "\r\n",url,host);
return _httpSyncExchange(host,buf,strlen(buf),ta,out);
}

int httpGetTest() {
char *out = 0;

int r = httpSyncGet("ya.ru","/",10,&out);

printf("Res=%d Full=%s\n",r,out);
return 0;

}



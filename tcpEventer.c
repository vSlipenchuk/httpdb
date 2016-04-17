
#include "tcpEventer.h"
#include "httpSrv.h"

void tcpEventerDone(tcpEventer *e) {

}

VS0OBJ0(tcpEventer,tcpEventerDone);

int tcpEventerEchoPacket(Socket *sock,char *data,struct _tcpEventer *e) {
char buf[1024]; int i;
SocketPool *sp=sock->pool;
 printf("tcpEventerEchoPacket:%s\n",data);
sprintf(buf,"YOU_SEND_LINE:%s\n",data);  SocketSend(sock,buf,strlen(buf));
sprintf(buf,"EVT:%s\n",data);
for(i=0;i<arrLength(sp->sock);i++) {
    Socket *s = sp->sock[i];
    if (s->state !=sockListen && s!=sock) {
         SocketSend(s,buf,strlen(buf));
      }
    }
return 0;
}

tcpEventer *TheEventer;

tcpEventer *tcpEventerCreate(char *ini) { // Создание соединения по умолчанию для eventer
tcpEventer *srv;
srv = tcpEventerNew(); TheEventer = srv; // fix last eventer
TimeUpdate();
strcpy(srv->name,"tcpEventerSrv"); srv->logLevel = 6; // Debug !!!
strcpy(srv->srv.name,"eventer"); srv->srv.logLevel = 6;  // LoggerName
srv->keepAlive = 1; // Yes, keep it
srv->created = TimeNow;
srv->readLimit.Limit = 1000000; // 1 mln в секунду -)))
srv->onPacket = tcpEventerEchoPacket;
return srv;
}

int TheTcpEventerRun() {
if (!TheEventer) return 0;
return SocketPoolRun(&TheEventer->srv);
}

int tcpEventerProcess(tcpEventer *srv) { // Dead loop till the end???
while(!aborted) {
  TimeUpdate(); // TimeNow & szTimeNow
  int cnt = SocketPoolRun(&srv->srv);
  //printf("SockPoolRun=%d time:%s\n",cnt,szTimeNow); msleep(1000);
  RunSleep(cnt); // Empty socket circle -)))
  if (srv->runTill && TimeNow>=srv->runTill) break; // Done???
  }
return 0;
}


// -- acception new connections

int onEventerPacket(uchar *data,int len, Socket *sock) { // CHeck - if packet ready???
    int i;
//vssHttp req;
for(i=0;i<len;i++) if (data[i]=='\n') break;
if (data[i]!='\n') return 0; // not ready yet
len = i; data[i]=0; len++;
tcpEventer *srv = (void*)sock->pool; // Есть, у меня есть счетчик пакетов - прямо на сокете???
CLOG(srv,4,"new eventRequest#%d/%d client '%s'\n",sock->N,sock->recvNo,sock->szip);
CLOG(srv,6,"new eventRequest#%d/%d requestBody '%s'\n",sock->N,sock->recvNo,data);
// Сначала - пытаемся найти мапу. Для этого - нужно разобрать запрос
if (!srv->keepAlive) sock->dieOnSend = 1;
   srv->onPacket(sock,data,srv);
return len; // remove data from input queue and go ahead
}


int onTcpEventerConnect(Socket *lsock, int handle, int ip) {
Socket *sock;
tcpEventer *srv = (void*)lsock->pool; // Here My SocketServer ???
sock = SocketPoolAccept(&srv->srv,handle,ip);
CLOG(srv,3,"new connect#%d from '%s', accepted\n",sock->N,sock->szip);
sock->checkPacket = onEventerPacket; // When new packet here...
if (srv->readLimit.Limit) sock->readPacket = &srv->readLimit; // SetLimiter here (if any?)
return 1; // Everything is done...
}



/* Socket must be addref() and removed from a parent pool */
int onTcpEventerOrphan(tcpEventer *srv,Socket *sock) {
  // vecRemove((void***)&sp->sock,sock);
CLOG(srv,6,"SocketPoolAccept2:{n:%d,ip:'%s',sock:%d}\n",sock->N,sock->szip,sock->sock);
 Socket2Pool(sock,&srv->srv); // add it there
CLOG(srv,3,"new orphan connect#%d from '%s', accepted\n",sock->N,sock->szip);
 // remove from an old pool?
sock->checkPacket = onEventerPacket; // When new packet here...
if (srv->readLimit.Limit) sock->readPacket = &srv->readLimit; // SetLimiter here (if any?)
return 1; // Everything is done...
}

/**/

int onEventHttp(Socket *sock, vssHttp *req, SocketMap *map) { // Handler-Remover from a HTTP
char msg[256];
 SocketPool *p = (void*)sock->pool;
 httpSrv *srv =(void*)sock->pool;
strSetLength(&srv->buf,0); // ClearResulted
vss r=req->B;
if (TheEventer) { // remove
     printf("BeginEventer\n");
 objAddRef(sock);  vecRemove((void***)&p,sock);
     printf("Ready to include\n");
 onTcpEventerOrphan(TheEventer,sock);
     printf("Orphan done\n");
 SocketSendf(sock,"HTTP/1.1 200 OK\r\n\r\n");
} else {
 SocketSendf(sock,"HTTP/1.1 404 OK\r\n\r\n");
 }
/*
vssGetVar2Buffer(req->args,"msg",msg,sizeof(msg));
wsBroadcast((void*)sock->pool,msg); // all
SocketPrintHttp(sock,req,"+OK");
*/
return 1;
}


Socket *tcpEventerListen(tcpEventer *srv,int port) {
Socket *sock = SocketNew();
if (SocketListen(sock,port)<=0) {
    CLOG(srv,1,"-fail listen port %d",port);
    SocketClear(&sock); return 0; } // Fail Listen a socket???
CLOG(srv,2,"+listener started on port#%d",port);
sock->onConnect = onTcpEventerConnect;
Socket2Pool(sock,&srv->srv);
return sock;
}

// -- testing environment

tcpEventerMainBegin() {
 int logLevel=100; int port = 2020; int keepAlive=1; int Limit=1000;
net_init();
TimeUpdate();
tcpEventer *srv = tcpEventerCreate(0); // New Instance, no ini
srv->log =  srv->srv.log = logOpen("eventer.log"); // Create a logger
srv->logLevel = srv->srv.logLevel = logLevel;
srv->keepAlive=keepAlive;
srv->readLimit.Limit = Limit;
IFLOG(srv,0,"...starting eventer {port:%d,logLevel:%d,keepAlive:%d,Limit:%d},\n",
  port,logLevel,keepAlive,Limit);
//printf("...Creating a http server\n");


if (tcpEventerListen(srv,port)<=0) { // Starts listen port
   Logf("-FAIL start listener on port %d\n",port); return 1;
   }
Logf(".. listener is ready, Ctrl+C to abort\n");
//if (runTill) srv->runTill = TimeNow + runTill;
 //httpSrvProcess(srv); // Run All messages till CtrlC...
}


int tcpEventerMain(int npar,char **par) {
int i,Limit=1000000; int logLevel=10; int keepAlive=1; int port=2020;
if (npar==1) {
    printf("eventer.exe -p[port] -r[rootDir] -d[debugLevel] -m[ext=mime[&..]]) -L[limitPPS:1000000]\n");
    return 0;
    }
for(i=1;i<npar;i++) {
    char *cmd = par[i];
    if (*cmd=='-') cmd++;
    switch(*cmd) {
//    case 'p': sscanf(cmd+1,"%d",&port); break;
    //case 'S': sscanf(cmd+1,"%d",&sleepTime); break;
  //  case 'd': sscanf(cmd+1,"%d",&logLevel); break;
    //case 'k': sscanf(cmd+1,"%d",&keepAlive); break;
   // case 'T': sscanf(cmd+1,"%d",&runTill); break;
   // case 'L': sscanf(cmd+1,"%d",&Limit); break;
   // case 'r': rootDir=cmd+1; break;
   // case 'm': mimes = cmd+1; break;
    }
    }
net_init();
TimeUpdate();
tcpEventer *srv = tcpEventerCreate(0); // New Instance, no ini
srv->log =  srv->srv.log = logOpen("eventer.log"); // Create a logger
srv->logLevel = srv->srv.logLevel = logLevel;
srv->keepAlive=keepAlive;
srv->readLimit.Limit = Limit;
IFLOG(srv,0,"...starting eventer {port:%d,logLevel:%d,keepAlive:%d,Limit:%d},\n",
  port,logLevel,keepAlive,Limit);
//printf("...Creating a http server\n");


if (tcpEventerListen(srv,port)<=0) { // Starts listen port
   Logf("-FAIL start listener on port %d\n",port); return 1;
   }
Logf(".. listener is ready, Ctrl+C to abort\n");
//if (runTill) srv->runTill = TimeNow + runTill;
 //httpSrvProcess(srv); // Run All messages till CtrlC...

time_t *reported;

while(!aborted) {
  TimeUpdate(); // TimeNow & szTimeNow
  int cnt = SocketPoolRun(&srv->srv);
  /*if (reported != TimeNow) {
      reported = TimeNow;
      //wsBroadcast(&srv->srv, szTimeNow); // tmp - broadcast server time
     }*/
  //printf("SockPoolRun=%d time:%s\n",cnt,szTimeNow); msleep(1000);
  //RunSleep(cnt); // Empty socket circle -)))
  if (!cnt) msleep(10);


  if (srv->runTill && TimeNow>=srv->runTill) break; // Done???
  }


TimeUpdate();
IFLOG(srv,0,"...stop tcpEventer, done:{connects:%d,requests:%d,runtime:%d}\n",
     srv->srv.connects,srv->srv.requests,TimeNow - srv->created);
return 0;
}



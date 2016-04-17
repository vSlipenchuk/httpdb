#include <stdio.h>
#include <stdlib.h>
#include "../vos/vos.h"

#include "../vdb/vdb.h"
#include "httpSrv.h"
#include "vss.h"

#include "tcpEventer.h"

/*
-Wno-pointer-sign
-Wno-strict-aliasing
*/

/*

  some todo:
   1. db_schema for autoload urls
   2. js stub to access objects
   3. authorization for access (logins/acl)
   4. threads and async calls

*/

extern database *db; // on httpdb_var

void strCatQ(char **str,char *text,int len) { // cat quoted
if (len<0) len = strlen(text);
strCat(str,"\"",1);
for(;len>0;len--,text++) {
   if (*text=='"') {
       strCat(str,"\\\"",2);
       } else strCat(str,text,1);

  }
strCat(str,"\"",1);
}

void dbStrFetch(database *db,char **str) { // есть два варианта - и еще тема - если пустой селект !
while( db_fetch(db) ) {
  db_col *c; int i;
  strCat(str,"[",-1);
  for(i=0,c=db->out.cols;i<db->out.count;i++,c++) {
     strCat(str,c->name,-1);
     strCat(str,":",-1);
     char *t = db_text(c);
     if ( (c->type == dbInt) || (c->type == dbNumber)) {
     if (*t==0) strCat(str,"0",-1);
        else strCat(str,t,-1);
     } else {
        strCatQ(str,db_text(c),-1); // do it for a string...
      }
     if (i+1<db->out.count) strCat(str,",",-1);
     }
  strCat(str,"]",-1);
  }
}


int onDbRequest(Socket *sock, vssHttp *req, SocketMap *map) { // Генерация статистики по серверу
char buf[1024];
httpSrv *srv = (void*)sock->pool;
strSetLength(&srv->buf,0); // ClearResulted
vss r=req->B;
strCat(&srv->buf,r.data,r.len); char *sql = srv->buf;
printf("DB->REQ:<%*.*s>\n",VSS(req->B));
if (strncmp(r.data,"select",6)==0) { // get a data
    if (db_select(db,sql)<=0) { // error here ...
        SocketPrintHttp(sock,req,"-%s",db->error);
        return 1;
       }
  strSetLength(&srv->buf,0);
  dbStrFetch(db,&srv->buf);
  SocketSendHttp(sock,req,srv->buf,-1);
  return 1;
  } else { //exec a result
    if (db_exec_once(db,sql)<=0) { // error here
       SocketPrintHttp(sock,req,"-%s",db->error);
       db_rollback(db);
        return 1;
    }
    db_commit(db); // anyqway
    SocketPrintHttp(sock,req,"+1 OK");
        return 1;
  }
/*
sprintf(buf,"{clients:%d,connects:%d,requests:%d,mem:%d,serverTime:'%s',pps:%d}",arrLength(srv->srv.sock)-1,
  srv->srv.connects,
  srv->srv.requests,
  os_mem_used(), szTimeNow,
  (srv->readLimit.pValue+srv->readLimit.ppValue)/2);
*/
SocketPrintHttp(sock,req,"%s","-not implemented yet"); // Flash Results as httpreturn 1; // OK - generated
return 1;
}

int main(int npar,char **par) {

//    return httpGetTest();

    db = db_new();
    if (db_connect_string(db,"/@my.db#./sq3u.so")<=0) {
        printf("Fail connect to db err=%s\n",db->error);
        return 1;
       }
    printf("db connected OK\n");

//return tcpEventerMain(npar,par);

tcpEventerMainBegin(); // create an eventer
/*
    tcpEventer *e  = tcpEventerCreate("");
      e->logLevel=10; e->srv
      tcpEventerListen(e,2020); tcpEventerProcess(e); // TEST - forewer
*/

    return MicroHttpMain(npar,par);
    //printf("Hello world!\n");
    return 0;
}

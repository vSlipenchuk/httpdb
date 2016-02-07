#include "vdb.h"
#include "httpSrv.h"
#include "vss.h"

/*

Suppose usage of:
 1.
sqlite3 my.db << EOF
create table val(id char(20),name char(20),val char(80),modified datetime);
create unique index i_val on val(id,name);
insert into val(id,name,val,modified) values('001','myname','new_one',date('now'));
select * from val;
EOF
 2.

Usage wget syntax:
 1. /var.get?id=[id]&name=[name]
 2. /var.set?id=[id]&name=[name]&val=[val]

Test commands:
 1. wget "http://localhost/var.get?id=001&name=myname" -O- 2> /dev/null
 2. wget "http://localhost/var.set?id=001&name=myname&val=NewValue" -O- 2> /dev/null

*/

char * var_get_sql="select val from val where id=:id and name=:name";
char * var_set_sql="update val set val=:val,modified=date('now') where id=:id and name=:name";

database *db;

unsigned char *url_decode8(char *dst, unsigned char *src) { // Enocdes %XX and + in a http request headers ...
unsigned char *ret = dst;
while(*src) {
 if (*src=='+') *dst=' ';
  else if (*src=='%'&& hex(src[1])>=0 && hex(src[2])>=0)
    {
    int code=hex(src[1])*16+hex(src[2]);
    //if (code==0x85) *dst='.'; else
       *dst=code;
     src+=2;
    }
  else if (*src=='%' && src[1]=='u' && hex(src[2])>=0 && hex(src[3])>=0 && hex(src[4])>=0 && hex(src[5])>=0) {
    // Unicode Convert ...
    uchar wch[2];
    wch[0] = hex(src[4])*16+hex(src[5]);  // Code
    wch[1] = hex(src[2])*16+hex(src[3]);  // Lang
    //unicode_to_str(dst,1,wch,2); // ConvertSymbol to win2151? - oldalgo. need to fix.
     // now - skip... on after - to do ? unicode2utf8 ?
    src+=5;
    }
   else *dst=*src;
 dst++; src++;
 }
*dst=0;
return ret;
}



int vssGetVar2Buffer(vss arg,char *name,char *buf, int size) { // to do - url_decode_param?
int nl = strlen(name); vss A=arg;
memset(buf,0,size);
printf("A=<%*.*s>\n",VSS(A));
while (A.len>0) { // check it
  vss v = vssGetTillStr(&A,"&",1,1);
  vss n = vssGetTillStr(&v,"=",1,1);
  printf("Check Name=<%*.*s> V=<%*.*s> R=<%*.*s>\n",VSS(n),VSS(v),VSS(A));
  if (n.len == nl && memcmp(name,n.data,nl) == 0) { // found!
      size--; if (v.len>=size) v.len=size;
      memcpy(buf,v.data,v.len);
      url_decode8(buf,buf); // replace %20
      return 1;
     }
  }
printf("%s - not found\n",name);
return 0; // not found
}

int onDbVarGet(Socket *sock, vssHttp *req, SocketMap *map) { // Генерация статистики по серверу
char buf[1024],id[80],name[80];
httpSrv *srv = (void*)sock->pool;
strSetLength(&srv->buf,0); // ClearResulted
vss r=req->B;
vssGetVar2Buffer(req->args,"id",id,sizeof(id));
vssGetVar2Buffer(req->args,"name",name,sizeof(name));
//strCat(&srv->buf,r.data,r.len); char *sql = srv->buf;
printf("DB->REQ: get a var '%s' for id '%s'\n",name,id);
if (db_compile(db, var_get_sql)
      && db_bind(db,"id",dbChar,0,id,strlen(id))
      && db_bind(db,"name",dbChar,0,name,strlen(name))
      && db_open(db) && db_exec(db) && db_fetch(db) ) { // ok - respons a data
       printf("Yes!\n");
       char *t = db_text(db->out.cols);
       printf("Resp:%s\n",t);
       SocketSendHttp(sock,req,t,-1); // send result of first column
      } else { //error
          printf("ERR:%s\n",db->error);
        SocketPrintHttp(sock,req,"-%s",db->error);
      }
return 1;
}


int onDbVarSet(Socket *sock, vssHttp *req, SocketMap *map) { // Генерация статистики по серверу
char buf[1024],id[80],name[80],val[80];
httpSrv *srv = (void*)sock->pool;
strSetLength(&srv->buf,0); // ClearResulted
vss r=req->B;
vssGetVar2Buffer(req->args,"id",id,sizeof(id));
vssGetVar2Buffer(req->args,"name",name,sizeof(name));
vssGetVar2Buffer(req->args,"val",val,sizeof(val));
//strCat(&srv->buf,r.data,r.len); char *sql = srv->buf;
printf("DB->REQ: set a var '%s' to '%s' for id '%s'\n",name,val,id);
if (db_compile(db, var_set_sql)
      && db_bind(db,"id",dbChar,0,id,strlen(id))
      && db_bind(db,"name",dbChar,0,name,strlen(name))
      && db_bind(db,"val",dbChar,0,val,strlen(val))
      && db_exec(db) && db_commit(db)) { // ok - respons a data
       printf("set ok\n");
       SocketSendHttp(sock,req,"",-1); // send result of first column
      } else { //error
          printf("set ERR:%s\n",db->error);
        SocketPrintHttp(sock,req,"-%s",db->error);
      }
return 1;
}

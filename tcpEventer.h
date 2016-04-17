#ifndef TCP_EVENTER
#define TCP_EVENTER

#include "vos.h"
#include "sock.h"
#include "vss.h"


/*
  It is simple text line server
  with internal message resend
*/


typedef struct _tcpEventer {
    SocketPool srv; // Тут все сокеты (включая слушателя)
    char name[14]; int logLevel; logger *log; // Logging
    int (*onPacket)(Socket *sock,char *data,struct _tcpEventer *srv); // when packet fires on a server
//SocketMap **map; // Залинкованные URL (файловые и программные)
//    vssHttp req; // Текущий запрос на обработку - ???
    //uchar *index; // Кешированный индекс (отдается мгновенно - или сделать map?)
    uchar *buf; // Временный файл (для закачки файлов)
    //uchar *mimes;
    //httpMime *mime; // Пассивная строка и собственно - разобранные маймы для быстрого поиска
    Counter readLimit; // Limiter for incoming counts
    //SocketPool cli; // Клиенты - для редиректа???
    //vss defmime; // DefaultMime for a page ???
    int keepAlive; // Disconenct after send???
    time_t created; // When it has beed created -)))
    time_t runTill;
    void *handle; // any user defined handle
} tcpEventer;

VS0OBJH(tcpEventer);


tcpEventer *tcpEventerCreate(char *ini);
Socket *tcpEventerListen(tcpEventer *srv,int port);

#endif

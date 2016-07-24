#ifndef PUBLIC_H_
#define PUBLIC_H_


#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <sys/time.h>
#include <time.h>
#include <deque>
#include <set>
#include <map>
#include <list>


enum PROTOCOL
{
   PROTOUNKNOWN,   
   HTTP,
   HTTPS
};

enum DIRECTION
{
    DIRUNKNOWN,
    C2S,
    S2C
};

//define method
const static uint32_t    METUNKNOWN = 0;
const static uint32_t    GET = 1;
const static uint32_t    POST = 2;
const static uint32_t    OPTIONS = 3;
const static uint32_t    HEAD = 4;
const static uint32_t    PUT = 5;
const static uint32_t    DELETE = 6;
const static uint32_t    TRACE = 7; 
const static uint32_t    CONNECT = 8;

//define version
const static uint32_t    VERUNKNOWN = 0;
const static uint32_t    VER9 = 9;
const static uint32_t    VER10 = 10;
const static uint32_t    VER11 = 11;

//define timeout
const  static time_t   TIMEOUT_CHECK_DUR = 120;
const  static time_t   TIMEOUT = 3600;

//define match way
const  static uint32_t UNK_MATCH = 0; // unknown match
const  static uint32_t C2S_MATCH = 1; //client to server
const  static uint32_t S2C_MATCH = 2; //server to client


typedef std::map<std::string,std::string> HeaderMap;

struct CResult
{
    CResult()
    {
        Reset(); 
    }

    void Reset()
    {
        RequestTimeStamp = 0;
        ResponseTimeStamp = 0;
        Method = METUNKNOWN;
        RetCode = 0;
        Url = "";
        RetNote = "";
        Version = VERUNKNOWN;
        RequestHeader.clear();
        ResponseHeader.clear();
        RequestData = "";
        ResponseData = "";
    }


    uint64_t      RequestTimeStamp;
    uint64_t      ResponseTimeStamp;

    uint32_t      Method;
    uint32_t      Version;
    uint32_t      RetCode;
    std::string   Url;
    std::string   RetNote;
    HeaderMap     RequestHeader;
    HeaderMap     ResponseHeader;
    std::string   RequestData;
    std::string   ResponseData;
};
#endif

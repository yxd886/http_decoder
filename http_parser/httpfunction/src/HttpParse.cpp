#include <boost/lexical_cast.hpp>
#include "HttpParse.h"
#include <arpa/inet.h>


using namespace std;

CHttpParse::CHttpParse()
{
}

CHttpParse::~CHttpParse()
{

}

void CHttpParse::Init()
{
}

void CHttpParse::Parse(CSessionPtr& sesptr)
{
    uint32_t reqLen = 0;
    uint32_t rspLen = 0;
    const char*  reqBuf = sesptr->ReqBuf.GetBuf(reqLen);
    const char*  rspBuf = sesptr->RspBuf.GetBuf(rspLen);

    if(!reqBuf || !rspBuf || !reqLen || !rspLen)
    {
        sesptr->Reset(); 
        return;
    }


    uint32_t pos = 0;

    if(reqLen > 10)  //parse request packet
    {

    	printf("parsing MethodAndUri !\n");
    	if(!ParseMethodAndUri(reqBuf,reqLen,pos,sesptr->Result))  //parse method, uri, version
        {
    		printf("parsing MethodAndUri failure!\n");
    		sesptr->Reset();
            return; 
        }

    	printf("parsing Request header !\n");
    	if(!ParseHeader(reqBuf, reqLen,pos, sesptr->Result.RequestHeader))  //parse request header
        {
    		printf("parsing Request header failure!\n");
    		sesptr->Reset();
            return; 
        }
    	printf("parsing Request data !\n");
        if(!ParseReqData(reqBuf,reqLen,pos,sesptr->Result))  //parse request data
        {
        	printf("parsing Request data failure!\n");
        	sesptr->Reset();
            return; 
        }
    }

    if(rspLen > 10)  //parse response packet
    {
        pos = 0;
        printf("parsing Response state !\n");
        if(!ParseRspState(rspBuf,rspLen,pos,sesptr->Result)) //parse reponse state
        {
        	printf("parsing Response state failure!\n");
        	sesptr->Reset();
            return; 
        }

        printf("parsing Response header !\n");
        if(!ParseHeader(rspBuf,rspLen, pos,sesptr->Result.ResponseHeader)) //parse reponse header
        {
        	printf("parsing Response header failure!\n");
        	sesptr->Reset();
            return; 
        }

        printf("parsing Response data !\n");
        if(!ParseRspData(rspBuf,rspLen, pos,sesptr->Result)) //parse reponse data
        {
        	printf("parsing Response data failure!\n");
        	sesptr->Reset();
            return; 
        }
    }



    Send(sesptr);
    sesptr->Reset();

    return;
}

bool CHttpParse::ParseMethodAndUri(const char* pBuf, const uint32_t len, uint32_t& pos, CResult& result)
{
    //get method
    string method;
    int ret = GetBufByTag(pBuf+pos,len-pos," ",1,method);
    if( ret == -1)
    {
        //log get method error
    	printf("method prasing failure\n");
        return false; 
    }
    pos += ret;
    pos += 1;  //skip the space
    result.Method = GetMethod(method); 
    cout<<"Method:"<<method<<endl;


    //get url
    ret = GetBufByTag(pBuf+pos,len-pos," ",1,result.Url); 
    if(ret == -1)
    {
        //log get url error
    	printf("get url failure\n");
        return false; 
    }
    pos += ret;
    pos += 1;  //skip the space
    cout<<"Url:"<<result.Url<<endl;


    //get http version
    string version;
    ret = GetBufByTag(pBuf+pos,len-pos,"\n",1,version);
    if( ret == -1)
    {
        //log get version error
    	printf("get version failure\n");
        return false; 
    }
    pos += ret;
    pos += 1;  //skip the \r\n
    cout<<"Version:"<<version<<endl;
    if(!GetVersion(version,result.Version))
    {
    	printf(" version compare failure\n");
    	return false;
    }

    
    return true;

}

bool CHttpParse::ParseRspState(const char* pBuf, const uint32_t len,uint32_t& pos, CResult& result)
{
    int ret = 0;
        
    //check the version with reqeust version
    string version;
    uint32_t ver;
    ret = GetBufByTag(pBuf+pos,len-pos," ",1,version); 
    if( ret == -1)
    {
        //log reponse get version error
        return false; 
    }
    pos += ret;
    pos += 1;  //skip the space

    if(!GetVersion(version,ver))
    {
        return false; 
    }

    if( ver != result.Version)
    {
        //log response version is not equal to request version 
        return false;
    }


    //get the response code 
    string rspCode;
    ret = GetBufByTag(pBuf+pos,len-pos," ",1,rspCode); 
    if( ret == -1)
    {
        //log reponse get version error
        return false; 
    }
    pos += ret;
    pos += 1;  //skip the space

    try
    {
       result.RetCode  = boost::lexical_cast<uint32_t>(rspCode);
    }
    catch(boost::bad_lexical_cast& e)
    {
        //log error to cast retcode  %s <=> e.what() 
        return false;
    }

    
    ret = GetBufByTag(pBuf+pos,len-pos,"\r\n",2,result.RetNote);
    if( ret == -1)
    {
        //log reponse get version error
        return false; 
    }
    pos += ret;
    pos += 2;  //skip the \r\n 


    return true;
}

bool CHttpParse::ParseHeader(const char* pBuf, const uint32_t len,uint32_t& pos,  HeaderMap& headmap)
{
    int ret = 0;


    while(true)
    {
        string key = "";
        string value = "";

        if(!(pBuf+pos) || !(len-pos))
        {
            //log buf modified  %d <=> len
            return false; 
        }
        cout<<"pBuf + pos=";
        putchar(*(pBuf + pos));
        cout<<endl;
        if(strncmp(pBuf + pos,"\n",1) == 0)
        {
            pos += 1;
            return true; 
        }
        if(strncmp(pBuf + pos,"\r\n",2) == 0)
                {
                    pos += 2;
                    return true;
                }

        ret = GetBufByTag(pBuf + pos,len - pos,":",1,key);
        if( ret == -1)
        {
            //log parse header key error
            return false; 
            printf("find key failure\n");
        }
        pos += ret;
        pos += 1; //skip the :
        cout<<"key:"<<key<<endl;

        ret = GetBufByTag(pBuf + pos,len - pos,"\n",1,value);
        if( ret == -1)
        {
            //log parse header value error
        	printf("find value failure\n");
            return false; 
        }
        pos += ret;
        pos += 1; //skip the \r\n
        cout<<"value:"<<value<<endl;
        
        headmap[key] = value;
    }
    return true;
}

bool CHttpParse::ParseReqData(const char* pBuf, const uint32_t len,uint32_t& pos, CResult& result)
{
    return true;
}

bool CHttpParse::ParseRspData(const char* pBuf, const uint32_t len,uint32_t& pos, CResult& result)
{
    return true;
}

void CHttpParse::Send(CSessionPtr&  sesptr)
{
    string file("../../");
    try
    {
        file += boost::lexical_cast<string>(sesptr->ClientPort);
    }
    catch(boost::bad_lexical_cast& e)
    {
        //log error to cast version  %s <=> e.what() 
        return;
    }

    std::ofstream output(file.c_str(), std::ios::out | std::ios::app);
   // FILE *fp=fopen(file,)

    union iptrans p,q;
  // uint32_t a=htonl(sesptr->ClientIp);
  //  uint32_t b=htonl(sesptr->ServerIp);
    p.ip=sesptr->ClientIp;q.ip=sesptr->ServerIp;
    //output <<inet_ntoa(*((struct in_addr*)&a))<< ":" << sesptr->ClientPort << " ===> " << inet_ntoa(*(struct in_addr*)(&b)) << ":" << sesptr->ServerPort << std::endl;
    output <<(int)p.x4<<"."<<(int)p.x3<<"."<<(int)p.x2<<"."<<(int)p.x1 << ":" << sesptr->ClientPort << " ===> " <<(int)q.x4<<"."<<(int)q.x3<<"."<<(int)q.x2<<"."<<(int)q.x1  << ":" << sesptr->ServerPort << std::endl;
   // output <<sesptr->ClientIpstr<< ":" << sesptr->ClientPort << " ===> " << sesptr->ServerIpstr << ":" << sesptr->ServerPort << std::endl;
    output << "method: " << sesptr->Result.Method << "\tversion: " << sesptr->Result.Version << std::endl;
    output << "retrun code : " << sesptr->Result.RetCode << sesptr->Result.RetNote << std::endl;
    output << "url : " << sesptr->Result.Url << std::endl;
    output << "Request Header:\n";
    HeaderMap::iterator it = sesptr->Result.RequestHeader.begin();
    for(; it != sesptr->Result.RequestHeader.end(); it++)
    {
        output << "\t" <<  it->first << ":  " << it->second << "\n";
    }
    output << "Request Context:\n";
    unsigned int length;
    char*tmp = sesptr->ReqBuf.GetBuf(length);
    output.write(tmp,length);
    output<<std::endl;
    output << "Response Header:\n";
    HeaderMap::iterator itr = sesptr->Result.ResponseHeader.begin();
    for(; itr != sesptr->Result.ResponseHeader.end(); itr++)
    {
        output << "\t" <<  itr->first << ":  " << itr->second << "\n";
    }
    output << "Response Context:\n";

    tmp = sesptr->RspBuf.GetBuf(length);
    output.write(tmp,length);
    output<<std::endl;
    output << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    output.close();

    return;
}

bool CHttpParse::GetVersion(string version, uint32_t& ver)
{
    if(version.size() != 8)
    {
        //log error to get version. %s <=> version 
        return false;
    }

    if(version.find("HTTP/") == string::npos)
    {
        //log error to get version. %s <=> version 
        return false;
    }

    string tmp = version.substr(5,3);

    float i;

    try
    {
        i = boost::lexical_cast<float>(tmp);
    }
    catch(boost::bad_lexical_cast& e)
    {
        //log error to cast version  %s <=> e.what() 
        return false;
    }

    ver = (uint32_t) (i * 10);

    return true;
}

uint32_t CHttpParse::GetMethod(string method)
{
    if(method.compare("GET") == 0)
    {
        return GET; 
    }
    else if(method.compare("POST") == 0)
    {
        return POST; 
    }
    else if(method.compare("OPTIONS") == 0)
    {
        return OPTIONS; 
    }
    else if(method.compare("HEAD") == 0)
    {
        return HEAD; 
    }
    else if(method.compare("PUT") == 0)
    {
        return PUT; 
    }
    else if(method.compare("DELETE") == 0)
    {
        return DELETE; 
    }
    else if(method.compare("TRACE") == 0)
    {
        return TRACE; 
    }
    else if(method.compare("CONNECT") == 0)
    {
        return CONNECT; 
    }
    else
    {
        METUNKNOWN; 
    }
}

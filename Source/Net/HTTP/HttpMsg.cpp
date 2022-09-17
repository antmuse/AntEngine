/***************************************************************************************************
 * MIT License
 *
 * Copyright (c) 2021 antmuse@live.cn/antmuse@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
***************************************************************************************************/


#include "Net/HTTP/HttpMsg.h"
#include "Logger.h"
#include "Net/HTTP/HttpLayer.h"

namespace app {
namespace net {
static const s32 G_MAX_BODY = 1024 * 1024 * 10;

HttpMsg::HttpMsg(HttpLayer* it) :
    mStationID(0),
    mRespStatus(0),
    mLayer(it),
    mEvent(nullptr),
    mFlags(0),
    mMethod(HTTP_GET),
    mType(EHTTP_BOTH),
    mStatusCode(HTTP_STATUS_OK){
    //mBrief.setLen(0);
    mCacheIn.init();
    mCacheOut.init();
    if (it) {
        it->grab();
    }
}


HttpMsg::~HttpMsg() {
    setEvent(nullptr);
    if (mLayer) {
        mLayer->drop();
        mLayer = nullptr;
    }
}


void HttpMsg::writeStatus(s32 val, const s8* str) {
    s8 tmp[64]; //len=17="HTTP/1.1 200 OK\r\n"
    mStatusCode = val;
    usz tsz = snprintf(tmp, sizeof(tmp), "HTTP/1.1 %d %s\r\n", val, str);
    if (tsz >= sizeof(tmp)) {
        tsz = sizeof(tmp);
        tmp[tsz - 2] = '\r';
        tmp[tsz - 1] = '\n';
    }
    mCacheOut.write(tmp, static_cast<s32>(tsz));
}


void HttpMsg::writeOutBody(const void* buf, usz bsz) {
    mCacheOut.write(buf, bsz);
}


void HttpMsg::writeOutHead(const s8* name, const s8* value) {
    if (name && value) {
        usz klen = strlen(name);
        usz vlen = strlen(value);
        if (klen > 0) {
            mCacheOut.write(name, klen);
            mCacheOut.write(":", 1);
            mCacheOut.write(value, vlen);
            mCacheOut.write("\r\n", 2);
        }
    }
}


const StringView HttpMsg::getMimeType(const s8* filename, usz iLen) {
    DASSERT(filename && iLen > 0);
    /*
    *基于文件后缀的mime type表。
    *后缀必须按顺序排序，这样才能进行二分查找
    */
    static const struct {
        StringView mExt;
        StringView mMime;
    } TableMIME[] = {
        {{"ai", 2}, {"application/postscript", 22}},
        {{"aif", 3}, {"audio/x-aiff", 12}},
        {{"aifc", 4}, {"audio/x-aiff", 12}},
        {{"aiff", 4}, {"audio/x-aiff", 12}},
        {{"arj", 3}, {"application/x-arj-compressed", 28}},
        {{"asc", 3}, {"text/plain", 10}},
        {{"asf", 3}, {"video/x-ms-asf", 14}},
        {{"asx", 3}, {"video/x-ms-asx", 14}},
        {{"au", 2}, {"audio/ulaw", 10}},
        {{"avi", 3}, {"video/x-msvideo", 15}},
        {{"bat", 3}, {"application/x-msdos-program", 27}},
        {{"bcpio", 5}, {"application/x-bcpio", 19}},
        {{"bin", 3}, {"application/octet-stream", 24}},
        {{"c", 1}, {"text/plain", 10}},
        {{"cc", 2}, {"text/plain", 10}},
        {{"ccad", 4}, {"application/clariscad", 21}},
        {{"cdf", 3}, {"application/x-netcdf", 20}},
        {{"class", 5}, {"application/octet-stream", 24}},
        {{"cod", 3}, {"application/vnd.rim.cod", 23}},
        {{"com", 3}, {"application/x-msdos-program", 27}},
        {{"cpio", 4}, {"application/x-cpio", 18}},
        {{"cpp", 3}, {"text/plain", 10}},
        {{"cpt", 3}, {"application/mac-compactpro", 26}},
        {{"csh", 3}, {"application/x-csh", 17}},
        {{"css", 3}, {"text/css", 8}},
        {{"dcr", 3}, {"application/x-director", 22}},
        {{"deb", 3}, {"application/x-debian-package", 28}},
        {{"dir", 3}, {"application/x-director", 22}},
        {{"dl", 2}, {"video/dl", 8}},
        {{"dms", 3}, {"application/octet-stream", 24}},
        {{"doc", 3}, {"application/msword", 18}},
        {{"drw", 3}, {"application/drafting", 20}},
        {{"dvi", 3}, {"application/x-dvi", 17}},
        {{"dwg", 3}, {"application/acad", 16}},
        {{"dxf", 3}, {"application/dxf", 15}},
        {{"dxr", 3}, {"application/x-director", 22}},
        {{"eps", 3}, {"application/postscript", 22}},
        {{"etx", 3}, {"text/x-setext", 13}},
        {{"exe", 3}, {"application/octet-stream", 24}},
        {{"ez", 2}, {"application/andrew-inset", 24}},
        {{"f", 1}, {"text/plain", 10}},
        {{"f90", 3}, {"text/plain", 10}},
        {{"fli", 3}, {"video/fli", 9}},
        {{"flv", 3}, {"video/flv", 9}},
        {{"gif", 3}, {"image/gif", 9}},
        {{"gl", 2}, {"video/gl", 8}},
        {{"gtar", 4}, {"application/x-gtar", 18}},
        {{"gz", 2}, {"application/x-gzip", 18}},
        {{"hdf", 3}, {"application/x-hdf", 17}},
        {{"hh", 2}, {"text/plain", 10}},
        {{"hqx", 3}, {"application/mac-binhex40", 24}},
        {{"h", 1}, {"text/plain", 10}},
        {{"htm", 3}, {"text/html; charset=utf-8", 24}},
        {{"html", 4}, {"text/html; charset=utf-8", 24}},
        {{"ice", 3}, {"x-conference/x-cooltalk", 23}},
        {{"ico", 3}, {"image/x-icon", 12}},
        {{"ief", 3}, {"image/ief", 9}},
        {{"iges", 4}, {"model/iges", 10}},
        {{"igs", 3}, {"model/iges", 10}},
        {{"ips", 3}, {"application/x-ipscript", 22}},
        {{"ipx", 3}, {"application/x-ipix", 18}},
        {{"jad", 3}, {"text/vnd.sun.j2me.app-descriptor", 32}},
        {{"jar", 3}, {"application/java-archive", 24}},
        {{"jpeg", 4}, {"image/jpeg", 10}},
        {{"jpe", 3}, {"image/jpeg", 10}},
        {{"jpg", 3}, {"image/jpeg", 10}},
        {{"js", 2}, {"application/x-javascript", 24}},
        {{"json", 4}, {"application/json; charset=utf-8", 31}},
        {{"kar", 3}, {"audio/midi", 10}},
        {{"latex", 5}, {"application/x-latex", 19}},
        {{"lha", 3}, {"application/octet-stream", 24}},
        {{"lsp", 3}, {"application/x-lisp", 18}},
        {{"lzh", 3}, {"application/octet-stream", 24}},
        {{"m", 1}, {"text/plain", 10}},
        {{"m3u", 3}, {"audio/x-mpegurl", 15}},
        {{"man", 3}, {"application/x-troff-man", 23}},
        {{"me", 2}, {"application/x-troff-me", 22}},
        {{"mesh", 4}, {"model/mesh", 10}},
        {{"mid", 3}, {"audio/midi", 10}},
        {{"midi", 4}, {"audio/midi", 10}},
        {{"mif", 3}, {"application/x-mif", 17}},
        {{"mime", 4}, {"www/mime", 8}},
        {{"movie", 5}, {"video/x-sgi-movie", 17}},
        {{"mov", 3}, {"video/quicktime", 15}},
        {{"mp2", 3}, {"audio/mpeg", 10}},
        {{"mp2", 3}, {"video/mpeg", 10}},
        {{"mp3", 3}, {"audio/mpeg", 10}},
        {{"mpeg", 4}, {"video/mpeg", 10}},
        {{"mpe", 3}, {"video/mpeg", 10}},
        {{"mpga", 4}, {"audio/mpeg", 10}},
        {{"mpg", 3}, {"video/mpeg", 10}},
        {{"ms", 2}, {"application/x-troff-ms", 22}},
        {{"msh", 3}, {"model/mesh", 10}},
        {{"nc", 2}, {"application/x-netcdf", 20}},
        {{"oda", 3}, {"application/oda", 15}},
        {{"ogg", 3}, {"application/ogg", 15}},
        {{"ogm", 3}, {"application/ogg", 15}},
        {{"pbm", 3}, {"image/x-portable-bitmap", 23}},
        {{"pdb", 3}, {"chemical/x-pdb", 14}},
        {{"pdf", 3}, {"application/pdf", 15}},
        {{"pgm", 3}, {"image/x-portable-graymap", 24}},
        {{"pgn", 3}, {"application/x-chess-pgn", 23}},
        {{"pgp", 3}, {"application/pgp", 15}},
        {{"pl", 2}, {"application/x-perl", 18}},
        {{"pm", 2}, {"application/x-perl", 18}},
        {{"png", 3}, {"image/png", 9}},
        {{"pnm", 3}, {"image/x-portable-anymap", 23}},
        {{"pot", 3}, {"application/mspowerpoint", 24}},
        {{"ppm", 3}, {"image/x-portable-pixmap", 23}},
        {{"pps", 3}, {"application/mspowerpoint", 24}},
        {{"ppt", 3}, {"application/mspowerpoint", 24}},
        {{"ppz", 3}, {"application/mspowerpoint", 24}},
        {{"pre", 3}, {"application/x-freelance", 23}},
        {{"prt", 3}, {"application/pro_eng", 19}},
        {{"ps", 2}, {"application/postscript", 22}},
        {{"qt", 2}, {"video/quicktime", 15}},
        {{"ra", 2}, {"audio/x-realaudio", 17}},
        {{"ram", 3}, {"audio/x-pn-realaudio", 20}},
        {{"rar", 3}, {"application/x-rar-compressed", 28}},
        {{"ras", 3}, {"image/cmu-raster", 16}},
        {{"ras", 3}, {"image/x-cmu-raster", 18}},
        {{"rgb", 3}, {"image/x-rgb", 11}},
        {{"rm", 2}, {"audio/x-pn-realaudio", 20}},
        {{"roff", 4}, {"application/x-troff", 19}},
        {{"rpm", 3}, {"audio/x-pn-realaudio-plugin", 27}},
        {{"rtf", 3}, {"application/rtf", 15}},
        {{"rtf", 3}, {"text/rtf", 8}},
        {{"rtx", 3}, {"text/richtext", 13}},
        {{"scm", 3}, {"application/x-lotusscreencam", 28}},
        {{"set", 3}, {"application/set", 15}},
        {{"sgml", 4}, {"text/sgml", 9}},
        {{"sgm", 3}, {"text/sgml", 9}},
        {{"sh", 2}, {"application/x-sh", 16}},
        {{"shar", 4}, {"application/x-shar", 18}},
        {{"silo", 4}, {"model/mesh", 10}},
        {{"sit", 3}, {"application/x-stuffit", 21}},
        {{"skd", 3}, {"application/x-koan", 18}},
        {{"skm", 3}, {"application/x-koan", 18}},
        {{"skp", 3}, {"application/x-koan", 18}},
        {{"skt", 3}, {"application/x-koan", 18}},
        {{"smi", 3}, {"application/smil", 16}},
        {{"smil", 4}, {"application/smil", 16}},
        {{"snd", 3}, {"audio/basic", 11}},
        {{"sol", 3}, {"application/solids", 18}},
        {{"spl", 3}, {"application/x-futuresplash", 26}},
        {{"src", 3}, {"application/x-wais-source", 25}},
        {{"step", 4}, {"application/STEP", 16}},
        {{"stl", 3}, {"application/SLA", 15}},
        {{"stp", 3}, {"application/STEP", 16}},
        {{"sv4cpio", 7}, {"application/x-sv4cpio", 21}},
        {{"sv4crc", 6}, {"application/x-sv4crc", 20}},
        {{"svg", 3}, {"image/svg+xml", 13}},
        {{"swf", 3}, {"application/x-shockwave-flash", 29}},
        {{"t", 1}, {"application/x-troff", 19}},
        {{"tar", 3}, {"application/x-tar", 17}},
        {{"tcl", 3}, {"application/x-tcl", 17}},
        {{"tex", 3}, {"application/x-tex", 17}},
        {{"texi", 4}, {"application/x-texinfo", 21}},
        {{"texinfo", 7}, {"application/x-texinfo", 21}},
        {{"tgz", 3}, {"application/x-tar-gz", 20}},
        {{"tiff", 4}, {"image/tiff", 10}},
        {{"tif", 3}, {"image/tiff", 10}},
        {{"tr", 2}, {"application/x-troff", 19}},
        {{"tsi", 3}, {"audio/TSP-audio", 15}},
        {{"tsp", 3}, {"application/dsptype", 19}},
        {{"tsv", 3}, {"text/tab-separated-values", 25}},
        {{"txt", 3}, {"text/plain", 10}},
        {{"unv", 3}, {"application/i-deas", 18}},
        {{"ustar", 5}, {"application/x-ustar", 19}},
        {{"vcd", 3}, {"application/x-cdlink", 20}},
        {{"vda", 3}, {"application/vda", 15}},
        {{"viv", 3}, {"video/vnd.vivo", 14}},
        {{"vivo", 4}, {"video/vnd.vivo", 14}},
        {{"vrml", 4}, {"model/vrml", 10}},
        {{"vsix", 4}, {"application/vsix", 16}},
        {{"wav", 3}, {"audio/x-wav", 11}},
        {{"wax", 3}, {"audio/x-ms-wax", 14}},
        {{"wiki", 4}, {"application/x-fossil-wiki", 25}},
        {{"wma", 3}, {"audio/x-ms-wma", 14}},
        {{"wmv", 3}, {"video/x-ms-wmv", 14}},
        {{"wmx", 3}, {"video/x-ms-wmx", 14}},
        {{"wrl", 3}, {"model/vrml", 10}},
        {{"wvx", 3}, {"video/x-ms-wvx", 14}},
        {{"xbm", 3}, {"image/x-xbitmap", 15}},
        {{"xlc", 3}, {"application/vnd.ms-excel", 24}},
        {{"xll", 3}, {"application/vnd.ms-excel", 24}},
        {{"xlm", 3}, {"application/vnd.ms-excel", 24}},
        {{"xls", 3}, {"application/vnd.ms-excel", 24}},
        {{"xlw", 3}, {"application/vnd.ms-excel", 24}},
        {{"xml", 3}, {"text/xml", 8}},
        {{"xpm", 3}, {"image/x-xpixmap", 15}},
        {{"xwd", 3}, {"image/x-xwindowdump", 19}},
        {{"xyz", 3}, {"chemical/x-pdb", 14}},
        {{"zip", 3}, {"application/zip", 15}}
    };


    s8 suffix[20]; //����2,֧��<=18�ĺ�׺
    s32 pos = sizeof(suffix) - 1;
    suffix[pos--] = 0;
    ssz i;
    for (i = iLen - 1; pos >= 0 && i >= 0 && filename[i] != '.'; --i) {
        suffix[pos--] = App2Lower(filename[i]);
    }
    if (pos >= 0) {
        const s8* ext = suffix + pos + 1;
        const s32 extlen = sizeof(suffix) - pos - 2;
        ssz first = 0;
        ssz last = sizeof(TableMIME) / sizeof(TableMIME[0]) - 1;
        while (first <= last) {
            i = (first + last) / 2;
            s32 c = AppStrCMP(ext, TableMIME[i].mExt.mData, extlen);
            if (c == 0) {
                return TableMIME[i].mMime;
            }
            if (c < 0) {
                last = i - 1;
            } else {
                first = i + 1;
            }
        }
    }

    //default
    return StringView("application/octet-stream", sizeof("application/octet-stream") - 1);
}


s32 HttpMsg::writeGet(const String& req) {
    mCacheOut.reset();
    mURL.append(req.c_str(), req.getLen());
    if (!mURL.parser()) {
        return EE_ERROR;
    }
    StringView buf = mURL.getPath();
    mCacheOut.write("GET ", sizeof("GET ") - 1);
    mCacheOut.write(buf.mData, mURL.get().getLen() - (buf.mData - mURL.get().c_str()));
    mCacheOut.write(" HTTP/1.1\r\n", sizeof(" HTTP/1.1\r\n") - 1);

    buf = mURL.getHost();
    mCacheOut.write("Host:", sizeof("Host:") - 1);
    mCacheOut.write(buf.mData,buf.mLen);
    mCacheOut.write("\r\n", sizeof("\r\n") - 1);

    dumpHead(mHeadIn, mCacheOut);
    mCacheOut.write("\r\n", sizeof("\r\n") - 1);
    return EE_OK;
}



void HttpMsg::dumpHead(const HttpHead& hds, RingBuffer& out) {
    usz mx = hds.size();
    for (usz i = 0; i < mx; ++i) {
        const HeadLine& line = hds[i];
        out.write(line.mKey.c_str(), line.mKey.getLen());
        out.write(":", 1);
        out.write(line.mVal.c_str(), line.mVal.getLen());
        out.write("\r\n", sizeof("\r\n") - 1);
    }
}



}//namespace net
}//namespace app

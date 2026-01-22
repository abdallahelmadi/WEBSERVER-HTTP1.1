#pragma once
#include <iostream>

class type {
  public:
    static std::string get(std::string const& path) {
      std::string lastDot;
      size_t pos = path.find_last_of(".");
      if (pos != std::string::npos) {
        lastDot = path.substr(pos);
        if (lastDot == ".html" || lastDot == ".htm")
          return "text/html";
        else if (lastDot == ".css")
          return "text/css";
        else if (lastDot == ".js")
          return "application/javascript";
        else if (lastDot == ".json")
          return "application/json";
        else if (lastDot == ".xml")
          return "application/xml";
        else if (lastDot == ".png")
          return "image/png";
        else if (lastDot == ".jpg" || lastDot == ".jpeg")
          return "image/jpeg";
        else if (lastDot == ".gif")
          return "image/gif";
        else if (lastDot == ".svg")
          return "image/svg+xml";
        else if (lastDot == ".ico")
          return "image/x-icon";
        else if (lastDot == ".webp")
          return "image/webp";
        else if (lastDot == ".bmp")
          return "image/bmp";
        else if (lastDot == ".tiff" || lastDot == ".tif")
          return "image/tiff";
        else if (lastDot == ".mp3")
          return "audio/mpeg";
        else if (lastDot == ".wav")
          return "audio/wav";
        else if (lastDot == ".ogg")
          return "audio/ogg";
        else if (lastDot == ".aac")
          return "audio/aac";
        else if (lastDot == ".m4a")
          return "audio/mp4";
        else if (lastDot == ".flac")
          return "audio/flac";
        else if (lastDot == ".mp4")
          return "video/mp4";
        else if (lastDot == ".avi")
          return "video/x-msvideo";
        else if (lastDot == ".mov")
          return "video/quicktime";
        else if (lastDot == ".wmv")
          return "video/x-ms-wmv";
        else if (lastDot == ".flv")
          return "video/x-flv";
        else if (lastDot == ".webm")
          return "video/webm";
        else if (lastDot == ".mkv")
          return "video/x-matroska";
        else if (lastDot == ".mpeg" || lastDot == ".mpg")
          return "video/mpeg";
        else if (lastDot == ".txt")
          return "text/plain";
        else if (lastDot == ".pdf")
          return "application/pdf";
        else if (lastDot == ".doc")
          return "application/msword";
        else if (lastDot == ".docx")
          return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
        else if (lastDot == ".xls")
          return "application/vnd.ms-excel";
        else if (lastDot == ".xlsx")
          return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
        else if (lastDot == ".ppt")
          return "application/vnd.ms-powerpoint";
        else if (lastDot == ".pptx")
          return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
        else if (lastDot == ".odt")
          return "application/vnd.oasis.opendocument.text";
        else if (lastDot == ".ods")
          return "application/vnd.oasis.opendocument.spreadsheet";
        else if (lastDot == ".odp")
          return "application/vnd.oasis.opendocument.presentation";
        else if (lastDot == ".zip")
          return "application/zip";
        else if (lastDot == ".rar")
          return "application/x-rar-compressed";
        else if (lastDot == ".tar")
          return "application/x-tar";
        else if (lastDot == ".gz")
          return "application/gzip";
        else if (lastDot == ".7z")
          return "application/x-7z-compressed";
        else if (lastDot == ".bz2")
          return "application/x-bzip2";
        else if (lastDot == ".ttf")
          return "font/ttf";
        else if (lastDot == ".otf")
          return "font/otf";
        else if (lastDot == ".woff")
          return "font/woff";
        else if (lastDot == ".woff2")
          return "font/woff2";
        else if (lastDot == ".eot")
          return "application/vnd.ms-fontobject";
        else if (lastDot == ".bin")
          return "application/octet-stream";
        else if (lastDot == ".exe")
          return "application/x-msdownload";
        else if (lastDot == ".sh")
          return "application/x-sh";
        else if (lastDot == ".csv")
          return "text/csv";
        else if (lastDot == ".rtf")
          return "application/rtf";
        else if (lastDot == ".swf")
          return "application/x-shockwave-flash";
      }
      return "text/plain";
    }
};
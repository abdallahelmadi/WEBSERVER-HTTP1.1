#include <string>
#include <request.hpp>
#include <server.hpp>
#include <fstream>

int handle_multipart(const std::string& content, request& req, ctr& currentServer)
{
    std::string boundaryname = req.getHeaders().at("Content-Type");
    std::size_t boundary_pos = boundaryname.find("boundary=");
    if (boundary_pos == std::string::npos)
        return -1;

    std::string boundary = "--" + boundaryname.substr(boundary_pos + 9);
    std::string boundary_break = boundary + "--";

    std::size_t i = 0;
    while (i < content.length())
    {
        std::size_t boundary_start = content.find(boundary, i);
        if (boundary_start == std::string::npos)
            break;

        if (content.compare(boundary_start, boundary_break.length(), boundary_break) == 0)
            break;

        std::size_t part_start = boundary_start + boundary.length() + 2; // skip \r\n
        std::size_t boundary_end = content.find(boundary, part_start);
        if (boundary_end == std::string::npos)
            break;

        std::string part = content.substr(part_start, boundary_end - part_start);

        if (part.find("Content-Disposition: form-data;") != std::string::npos)
        {
            std::size_t filename_pos = part.find("filename=\"");
            if (filename_pos != std::string::npos)
            {
                std::size_t start = filename_pos + 10;
                std::size_t end = part.find("\"", start);
                if (end == std::string::npos)
                    return -1;

                std::string filename = part.substr(start, end - start);

                std::size_t data_start = part.find("\r\n\r\n");
                if (data_start != std::string::npos)
                {
                    std::string filedata = part.substr(
                        data_start + 4,
                        part.length() - (data_start + 4) - 2
                    );

                    std::string filepath = currentServer.uploaddir() + filename;

                    std::ofstream outfile(filepath.c_str(), std::ios::binary);
                    outfile.write(filedata.data(), filedata.size());
                    outfile.close();
                }
            }
            else
            {
                std::size_t name_pos = part.find("name=\"");
                if (name_pos != std::string::npos)
                {
                    std::size_t start = name_pos + 6;
                    std::size_t end = part.find("\"", start);
                    if (end == std::string::npos)
                        return -1;

                    std::string name = part.substr(start, end - start);

                    std::size_t data_start = part.find("\r\n\r\n");
                    if (data_start != std::string::npos)
                    {
                        std::string fielddata = part.substr(
                            data_start + 4,
                            part.length() - (data_start + 4) - 2
                        );

                        std::string stored_data =
                            "Field Name: " + name + ", Value: " + fielddata + "\n";
                        std::ofstream outfile(
                            (currentServer.uploaddir() + "form_fields.txt").c_str(),
                            std::ios::app
                        );
                        outfile << stored_data;
                        outfile.close();
                    }
                }
            }
        }
        i = boundary_end;
    }

    return 0;
}


void handle_json(const std::string& content , ctr& currentServer) {
  std::ofstream outfile( (currentServer.uploaddir() + "data.json").c_str());
  outfile << content;
  outfile.close();
}
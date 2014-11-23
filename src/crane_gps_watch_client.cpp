// Copyright (C) 2014 mru@sisyphus.teil.cc
//
// linux client for crane gps watch, runtastic gps watch.
//


#include <iostream>
#include <memory>

#include <getopt.h>

#ifdef HAVE_CONFIG_H
# include "../config.h"
#else
# define PACKAGE_STRING "crane_gps_watch_client"
# define PACKAGE_VERSION "0.1-win"
# define PACKAGE_BUGREPORT "mru sisyphus.teil.cc"
#endif

#include "DataTypes.hpp"
#include "Callback.hpp"
#include "TcxWriter.hpp"
#include "DebugWriter.hpp"
#include "GpsLint.hpp"
#include "ImageWriter.hpp"
#include "Watch.hpp"
#include "SerialLink.hpp"
#include "ImageLink.hpp"



std::string format_date_filename() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    const size_t len_max = 1000;
    char buffer[len_max];
    std::strftime(buffer, len_max, "%Y-%m-%d_%H-%M-%S", &tm);
    return buffer;
}

int main(int argc, char** argv) {

    std::string default_device;
#ifdef __MINGW32__
    default_device = "COM3";
#else
    default_device = "/dev/ttyUSB0";
#endif

    try {

        bool do_lint = false;
        bool split_by_track = false;
        int debug_level = 0;

        std::string from_image;
        std::string to_image;
        std::string device_fn;
        std::string output_fn;
        opterr = 0;
        while (1) {
            static struct option long_options[] = {
                  {"help", no_argument, 0, 'h'},
                  {"output", required_argument, 0, 'f'},
                  {"device", required_argument, 0, 'd'},
                  {"from_image", required_argument, 0, 'i'},
                  {"to_image", required_argument, 0, 't'},
                  {"split", no_argument, 0, 's'},
                  {"verbose", no_argument, 0, 'v'},
                  {"version", no_argument, 0, 'w'},
                  {"lint", no_argument, 0, 'l'},
                  {0, 0, 0, 0}
            };

            int option_index = 0;
            int c = getopt_long(argc, argv, "h", long_options, &option_index);
            if (c == -1) break;

            switch (c) {
              case 'h':
                std::cerr <<
                  PACKAGE_STRING "\n"
                  "crane_gps_watch_client --help\n"
                  "\n"
                  "crane_gps_watch_client [--output output-filename | --split] [--device " << default_device << " | --from_image image-file] [--to_image image-file] [--verbose]\n" 
                  "\n"
                  "See README.md or https://github.com/mru00/crane_gps_watch for details\n"
                  "Send bugreports to " PACKAGE_BUGREPORT
                  << std::endl;
                exit (0);
              case 'w':
                std::cerr <<
                  PACKAGE_STRING
                  << std::endl;
                exit (0);
              case 'd':
                if (!from_image.empty()) {
                    throw std::runtime_error("cannot use --device and --from_image at the same time");
                }
                device_fn = optarg;
                break;
              case 's':
                if (!output_fn.empty()) {
                    throw std::runtime_error("cannot use --output and --split at the same time");
                }
                split_by_track = true;
                break;
              case 'f':
                if (split_by_track) {
                    throw std::runtime_error("cannot use --output and --split at the same time");
                }
                output_fn = optarg;
                break;
              case 'i':
                if (!device_fn.empty()) {
                    throw std::runtime_error("cannot use --device and --from_image at the same time");
                }
                from_image = optarg;
                break;
              case 't':
                to_image = optarg;
                break;
              case 'v':
                debug_level++;
                break;
              case 'l':
                do_lint = true;
                break;
              default:
                std::cerr << "unknown option" << std::endl;
                exit(1);
            }
        }

        if (device_fn.empty()) {
            device_fn = default_device;
        }

        if (output_fn.empty()) {
            // set default output filename
            output_fn = format_date_filename() + ".tcx";
        }


        std::shared_ptr<DeviceInterface> device;
        if (from_image.empty()) {
            device = std::make_shared<SerialLink>(device_fn);
        }
        else {
            device = std::make_shared<ImageLink>(from_image);
        }

        Watch watch(device);

        watch.addRecipient(std::make_shared<TcxWriter>(output_fn, split_by_track));
        if (!to_image.empty()) {
            watch.addRecipient(std::make_shared<ImageWriter>(to_image));
        }
        if (debug_level > 0) {
            watch.addRecipient(std::make_shared<DebugWriter>(debug_level));
        }
        if (do_lint) {
            watch.addRecipient(std::make_shared<GpsLint>());
        }

        watch.parse();


        return 0;

    }
    catch(const std::runtime_error& error) {
        std::cerr << "error: " << error.what() << std::endl << "terminating" << std::endl;
        return 1;
    }
}

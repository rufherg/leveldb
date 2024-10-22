// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <cstdio>
#include <fstream>
#include <utility>

#include "leveldb/dumpfile.h"
#include "leveldb/env.h"
#include "leveldb/status.h"

namespace leveldb {
namespace {

class FilePrinter : public WritableFile {
public:
  explicit FilePrinter(const char* filepath) : file_path(filepath) {
    file = fopen(filepath, "w");
  }
  ~FilePrinter() override {
    fclose(file);
  }
  Status Append(const Slice& data) override {
    fwrite(data.data(), 1, data.size(), file);
    return Status::OK();
  }
  Status Close() override { return Status::OK(); }
  Status Flush() override { return Status::OK(); }
  Status Sync() override { return Status::OK(); }

private:
  const char* file_path;
  FILE *file;
};

class StdoutPrinter : public WritableFile {
 public:
  StdoutPrinter(const char* filename) { file = fopen(filename, "w");
  }

  ~StdoutPrinter() override { fclose(file); }

  Status Append(const Slice& data) override {
    fwrite(data.data(), 1, data.size(), stdout);
    fwrite(data.data(), 1, data.size(), file);
    return Status::OK();
  }
  Status Close() override { return Status::OK(); }
  Status Flush() override { return Status::OK(); }
  Status Sync() override { return Status::OK(); }

 private:
   FILE *file;
};

std::string GetFileNameFromPath(const std::string& path) {
  size_t found = path.find_last_of("/\\");
  if (found != std::string::npos) {
    return path.substr(found + 1);
  }
  return path;
}

bool HandleDumpCommand(Env* env, char** files, int num) {
  bool ok = true;
  for (int i = 0; i < num; i++) {
    std::string filename;
    filename.append(files[i]);
    filename.append("_output.txt");
    StdoutPrinter printer(filename.c_str());
    Status s = DumpFile(env, files[i], &printer);
    if (!s.ok()) {
      std::fprintf(stderr, "%s\n", s.ToString().c_str());
      ok = false;
    }
  }
  return ok;
}

bool HandleDumpFileCommand(Env* env, char** files, int num, char* filepath) {
  bool ok = true;
  for (int i = 0; i < num; i++) {
    std::string path(filepath);
    path.append(GetFileNameFromPath(std::string(files[i])));
    path.append("_output.txt");
    FilePrinter printer(path.c_str());
    Status s = DumpFile(env, files[i], &printer);
    if (!s.ok()) {
      std::fprintf(stderr, "%s\n", s.ToString().c_str());
      ok = false;
    }
  }
  return ok;
}

}  // namespace
}  // namespace leveldb

static void Usage() {
  std::fprintf(
      stderr,
      "Usage: leveldbutil command...\n"
      "   --dump files...                    -- dump contents of specified files\n"
      "   --dump files... --path filepath    -- dump contents to target path\n\n"
      "eg. leveldbutil.exe --dump C://xxx.com.leveldb/000003.log --path D://output/\n");
}

int main(int argc, char** argv) {
  leveldb::Env* env = leveldb::Env::Default();
  bool ok = true;
  if (argc < 2) {
    Usage();
    ok = false;
  } else {
    std::string command = argv[1];
    if (command == "--dump") {
      std::string command2 = argv[argc-2];
      if (command2 == "--path") {
        char* filepath = argv[argc-1];
        ok = leveldb::HandleDumpFileCommand(env, argv + 2, argc - 4, filepath);
      } else {
        ok = leveldb::HandleDumpCommand(env, argv + 2, argc - 2);
      }
    } else {
      Usage();
      ok = false;
    }
  }
  return (ok ? 0 : 1);
}

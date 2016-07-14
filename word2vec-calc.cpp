/* 
  Copyright(C) 2014 Naoya Murakami <naoya@createfield.com>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file includes the original word2vec's code. https://code.google.com/p/word2vec/
  The following is the header of the file:

    Copyright 2013 Google Inc. All Rights Reserved.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <vector>

#include <math.h>
#include <malloc.h>

#include <unicode/normlzr.h>
#include <re2/re2.h>
#include <google/gflags.h>

DEFINE_string(file_path, "", "Input binary file learned by word2vec");
DEFINE_string(input, "", "Input expression string");
DEFINE_int32(output, 1, "1:word,discatnce 2:word 3:split by commma 4:split by tab");
DEFINE_int32(offset, 0, "offset");
DEFINE_int32(limit, -1, "limit -1:all");
DEFINE_double(threshold, 0, "threshold (ex. --threshold 0.75)");
DEFINE_bool(no_normalize, false, "Don't use NFKC normalize");
DEFINE_string(term_filter, "", "Filter by regular expresion pattern to term");
DEFINE_string(output_filter, "", "Cut by regular expresion pattern to term");
DEFINE_bool(server, false, "Server mode");
DEFINE_string(ip, "", "IP address");
DEFINE_string(port, "", "Port number");
DEFINE_bool(h, false, "Help message");

using namespace std;

const long long max_size = 2000;         // max length of strings
const long long N = 40;                  // number of closest words that will be shown
const long long max_w = 50;              // max length of vocabulary entries

long long words, size = 0;
float *M = NULL;
char *vocab = NULL;

string
normalize(const string str)
{
  icu::UnicodeString src;
  icu::UnicodeString dest;
  src = str.c_str();
  UErrorCode status;
  status = U_ZERO_ERROR;
  Normalizer::normalize(src, UNORM_NFKC, 0, dest, status);
  if(U_FAILURE(status)) {
    throw runtime_error("Unicode normalization failed");
  }
  string result;
  dest.toUTF8String(result);
  transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

bool
word2vec_load(const char *file_name)
{
  FILE *f;
  float len;
  long long a, b;
  char ch;

  f = fopen(file_name, "rb");
  if (f == NULL) {
    printf("Input file not found : %s\n", file_name);
    return false;
  }

  fscanf(f, "%lld", &words);
  fscanf(f, "%lld", &size);
  vocab = (char *)malloc((long long)words * max_w * sizeof(char));
  M = (float *)malloc((long long)words * (long long)size * sizeof(float));
  if (M == NULL) {
    printf("Cannot allocate memory: %lld MB %lld %lld", 
           (long long)words * size * sizeof(float) / 1048576, words, size);
    return false;
  }
  for (b = 0; b < words; b++) {
    fscanf(f, "%s%c", &vocab[b * max_w], &ch);
    for (a = 0; a < size; a++) fread(&M[a + b * size], sizeof(float), 1, f);
    len = 0;
    for (a = 0; a < size; a++) len += M[a + b * size] * M[a + b * size];
    len = sqrt(len);
    for (a = 0; a < size; a++) M[a + b * size] /= len;
  }
  fclose(f);

  return true;
}

vector<string>
split(const string &str, char delim){
  vector<string> res;
  size_t current = 0, found;
  while((found = str.find_first_of(delim, current)) != string::npos){
    res.push_back(string(str, current, found - current));
    current = found + 1;
  }
  res.push_back(string(str, current, str.size() - current));
  return res;
}

int
calc(string str, int output, int offset, int limit, double threshold,
     bool no_normalize, const char *term_filter, const char *output_filter){
  char st1[max_size];
  char bestw[N][max_size];
  char st[100][max_size];
  char op[100];
  float dist, len, bestd[N], vec[max_size];
  long long a, b, c, d, cn, bi[100];

  for (a = 0; a < N; a++) bestd[a] = 0;
  for (a = 0; a < N; a++) bestw[a][0] = 0;

  st1[0] = 0;

  for(unsigned int i = 0; i < 100; i++){
    st[i][0] = 0;
    op[i] = '+';
  }

  if(!no_normalize){
    str = normalize(str);
  }
  re2::RE2::GlobalReplace(&str, "　", " ");

  vector<string> result_array;
  result_array = split(str, ' ');
  string result;

  int op_row = 1;
  for(unsigned int i = 0; i < result_array.size(); ++i )
  {
    if(result_array[i] == "+"){
      op[op_row] = '+';
      op_row++;
    }
    else if(result_array[i] == "-"){
      op[op_row] = '-';
      op_row++;
    } else {
      result += result_array[i];
      if ( i < result_array.size() - 1) {
        result += " ";
      }
    }
  }
  strcat(st1, result.c_str());

  cn = 0;
  b = 0;
  c = 0;
  while (1) {
    st[cn][b] = st1[c];
    b++;
    c++;
    st[cn][b] = 0;
    if (st1[c] == 0) break;
    if (st1[c] == ' ') {
      cn++;
      b = 0;
      c++;
    }
  }
  cn++;

  for (a = 0; a < cn; a++) {
    for (b = 0; b < words; b++) if (!strcmp(&vocab[b * max_w], st[a])) break;
    if (b == words) b = 0;
    bi[a] = b;
    if (output == 1) {
      printf("\nWord: %s  Position in vocabulary: %lld\n", st[a], bi[a]);
    }
    if (b == 0) {
      if (output == 1 || output == 2) {
        printf("Out of dictionary word!\n");
      }
      break;
    }
  }
  if (b == 0) return -1;
  if(cn == 1) {
    for (a = 0; a < size; a++) vec[a] = 0;
    for (b = 0; b < cn; b++) {
      if (bi[b] == -1) continue;
      for (a = 0; a < size; a++) vec[a] += M[a + bi[b] * size];
    }
  } else {
    for (a = 0; a < size; a++) vec[a] = 0;
    for (a = 0; a < size; a++) {
      for (b = 0; b < cn; b++) {
        if(op[b] == '-') {
          vec[a] -= M[a + bi[b] * size];
        } else {
          vec[a] += M[a + bi[b] * size];
        }
      }
    }
  }

  len = 0;
  for (a = 0; a < size; a++) len += vec[a] * vec[a];
  len = sqrt(len);
  for (a = 0; a < size; a++) vec[a] /= len;
  for (a = 0; a < N; a++) bestd[a] = 0;
  for (a = 0; a < N; a++) bestw[a][0] = 0;
  for (c = 0; c < words; c++) {
    a = 0;
    for (b = 0; b < cn; b++) if (bi[b] == c) a = 1;
    if (a == 1) continue;
    dist = 0;
    for (a = 0; a < size; a++) dist += vec[a] * M[a + c * size];
    for (a = 0; a < N; a++) {
      if (dist > bestd[a]) {
        if (threshold > 0 && dist < threshold) {
          break;
        }
        if (term_filter != NULL) {
          string s = &vocab[c * max_w];
          string t = term_filter;
          if ( RE2::FullMatch(s, t) ) {
            break;
          }
        }
        for (d = N - 1; d > a; d--) {
          bestd[d] = bestd[d - 1];
          strcpy(bestw[d], bestw[d - 1]);
        }
        bestd[a] = dist;
        strcpy(bestw[a], &vocab[c * max_w]);
        break;
      }
    }
  }

  unsigned int max;
  if (offset + limit > N) {
    max = N;
  } else {
    max = offset + limit;
  }

  for (a = offset; a < max; a++) {
    if (output_filter != NULL) {
      string s = bestw[a];
      re2::RE2::GlobalReplace(&s, output_filter, "");
      strcpy(bestw[a],s.c_str());
    }
    if ( a == 0 ) {
        switch(output) {
          case 3 :
            printf("%s", str.c_str());
            printf(",");
            break;
          case 4 :
            printf("%s", str.c_str());
            printf("\t");
            // for Groonga Groongaのシノニムは自身を含める必要あり。
            if(cn == 1) {
              printf("%s", str.c_str());
              printf("\t");
            }
            break;
        }
    }
    if (strlen(bestw[a]) > 0 && bestd[a] != 0) {
      if ( a < max - 1){
        switch(output) {
          case 1 :
            printf("%f\t%s\n", bestd[a], bestw[a]);
            break;
          case 2 :
            printf("%s\n", bestw[a]);
            break;
          case 3 :
            printf("%s", bestw[a]);
            printf(",");
            break;
          case 4 :
            printf("%s", bestw[a]);
            printf("\t");
            break;
        }
      } else {
        switch(output) {
          case 1 :
            printf("%f\t%s\n", bestd[a], bestw[a]);
            break;
          case 2 :
            printf("%s\n", bestw[a]);
            break;
          case 3 :
            printf("%s", bestw[a]);
            break;
          case 4 :
            printf("%s", bestw[a]);
            break;
        }
      }
    } else {
      if (max < N) {
        max++;
      }
    }
  }
  printf("\n");

  return 0;
}

int
main(int argc, char **argv) {
  const char *file_name = "/var/lib/word2vec/learn.bin";
  string str;

  google::ParseCommandLineFlags(&argc, &argv, true);

  if(FLAGS_h == true){
    printf(" --file_path : Input binary file learned by word2vec\n");
    printf(" --iuput : Input expression string\n");
    printf(" --output : 1:word,discatnce 2:word 3:split by commma 4:split by tab\n");
    printf(" --offset : offset\n");
    printf(" --limit : limit -1:all\n");
    printf(" --threshold : threshold (ex. --threshold 0.75)\n");
    printf(" --no_normalize : Don't use NFKC normalize\n");
    printf(" --term_filter : Filter by regular expresion pattern to term\n");
    printf(" --output_filter : Cut by regular expresion pattern to term\n");
    printf(" --server : Server mode\n");
    printf(" --ip : IP address\n");
    printf(" --port : Port number\n");
    exit(0);
  }

  if (FLAGS_file_path != "") {
    file_name = FLAGS_file_path.c_str();
  }

  if(word2vec_load(file_name) == false){
    printf("Load file failed\n");
    return -1;
  }
  
  int limit;
  if(FLAGS_limit == -1){
    limit = (int)N;
  } else {
    limit = FLAGS_limit;
  }
  if(FLAGS_input == ""){
    if(FLAGS_output == 1){
      printf("> ");
    }
    while(getline(cin, str)){
      if(str == "EXIT"){
        break;
      }
      calc(str, FLAGS_output, FLAGS_offset, limit, FLAGS_threshold,
           FLAGS_no_normalize, FLAGS_term_filter.c_str(),
           FLAGS_output_filter.c_str());
      if(FLAGS_output == 1){
        printf("> ");
      }
    }
  } else {
    if(FLAGS_output == 1){
      printf("> ");
    }
    ifstream ifs(FLAGS_input.c_str());
    if(ifs.fail()) {
      cerr << "File do not exist.\n";
      exit(0);
    }
    while(getline(ifs, str)){
      calc(str, FLAGS_output, FLAGS_offset, limit, FLAGS_threshold,
           FLAGS_no_normalize, FLAGS_term_filter.c_str(),
           FLAGS_output_filter.c_str());
      if(FLAGS_output == 1){
        printf("> ");
      }
    }
  }

  return 0;
}

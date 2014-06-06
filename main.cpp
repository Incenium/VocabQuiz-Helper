/*
===============================================================================
* File: main.cpp
*
* Author: Andrew Killian
*
* Email: Inceniium@gmail.com
*
* Description: Entry point for Quiz Helper, a program that gets your french
*              vocabulary words
===============================================================================
*/

//TODO: add error handling for when we don't find a word, eg. get the definition for the word that's closest to the specified word

//TODO: optimize the part of the code grabbing webpages, it can be excrutiatingly slow

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <curl/curl.h>
#include <curl/easy.h>

typedef struct word {
    std::string wordType;
    std::string wordDef;
} word;

// the definition tag
const static std::string DEF_TAG = "<li class=\"DivisionDefinition\">";

// the end definition tag
const static std::string END_DEF_TAG = "</li>";

// the inline example tag thingy
const static std::string EXAMPLE_TAG = "&nbsp;:";

// the word type tag, eg. noun, verb, etc.
const static std::string WORD_TYPE_TAG = "<p class=\"CatgramDefinition\">";

// the first word type end tag
const static std::string END_WORD_TYPE_TAG_1 = " <a class=\"lienconj\" href=\"/en/conjugation/french/";

// the second word type end tag
const static std::string END_WORD_TYPE_TAG_2 = "</p>";

// the tag which lists suggestions when a word is not found
// const static std::string NOT_FOUND_TAG;

// the dictionary link
const static std::string DIC_LINK = "http://www.larousse.com/en/dictionaries/french/";
/*
 * Name: curlWriter
 *
 * Parameter 1: callback function
 *
 * Returns: (int) success
 *
 * Description: writes html grabbed from a webpage to a buffer
 */
static int curlWriter(char* data, size_t size, size_t nmemb,
                      std::string *buffer)
{
    int result = 0;

    if (buffer != nullptr){
        buffer->append(data, size * nmemb);

        result = size * nmemb;
    }

    return result;
}

/*
 * Name: curlGetPage
 *
 * Parameter 1: (std::string) the link to a webpage
 *
 * Parameter 2: (char*) the error buffer curl will write errors to
 *
 * Parameter 3: (CURL*) the curl instance to use
 *
 * Returns: (CURLcode) success
 *
 * Description: grabs a webpage
 */
CURLcode curlGetPage(std::string link, std::string& buffer,
                      char* errorBuffer, CURL* curl)
{
    CURLcode res;

    if (curl){
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
        curl_easy_setopt(curl, CURLOPT_URL, link.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriter);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK){
            std::cerr << "curl_easy_perform() failed: " <<
                        curl_easy_strerror(res) << std::endl;
        }
    }

    return res;
}

/*
 * Name: parse
 *
 * Parameter 1: (std::string) the buffer that contains the html
 *
 * Returns: (word) definition and word type found in the html
 *
 * Description: parses the given html for the definition and word type
 */
word parse(std::string buffer)
{
    word w;
    size_t pos;

    // find the word type, eg. noun, verb, adverb, etc.
    if ((pos = buffer.find(WORD_TYPE_TAG)) != std::string::npos){
        size_t endpos;

        if ((endpos = buffer.find(END_WORD_TYPE_TAG_1, pos)) !=
            std::string::npos){

            w.wordType = buffer.substr(pos + WORD_TYPE_TAG.size(),
                                       endpos - (pos + WORD_TYPE_TAG.size()));
        }

        else if ((endpos = buffer.find(END_WORD_TYPE_TAG_2, pos)) !=
            std::string::npos){

            w.wordType = buffer.substr(pos + WORD_TYPE_TAG.size(),
                                       endpos - (pos + WORD_TYPE_TAG.size()));
        }
    }

    else
        w.wordDef = " ";

    // get the line with the definition in it
    if ((pos = buffer.find(DEF_TAG)) != std::string::npos){
        size_t endpos;

        if ((endpos = buffer.find(END_DEF_TAG, pos)) != std::string::npos){
            w.wordDef = buffer.substr(pos + DEF_TAG.size(),
                                      endpos - (pos + DEF_TAG.size()));
        }

        // loop over w.wordDef until all angle brackets and the content within
        // within them is erased
        while ((pos = w.wordDef.find("<")) != std::string::npos){
            size_t endpos;

            if (pos == 0){
                endpos = w.wordDef.find(">", w.wordDef.find(">"));
                w.wordDef.erase(pos, endpos - pos + 1);
            }

            if ((endpos = w.wordDef.find(">")) != std::string::npos)
                w.wordDef.erase(pos, endpos - pos + 1);
        }

        // check for the example tag
        if ((pos = w.wordDef.find(EXAMPLE_TAG)) != std::string::npos)
            w.wordDef.erase(pos, w.wordDef.size() - pos);
    }

    // strip the period from the end of the definition if it's there
    if (w.wordDef.at(w.wordDef.size() - 1) == '.')
        w.wordDef.erase(w.wordDef.size() - 1, 1);

    return w;
}

/*
 * Name: writeToFile
 *
 * Parameter 1: (std::string) word to write to file
 *
 * Parameter 2: (std::string) definition of the word to write to file
 *
 * Returns: (bool) success
 *
 * Description: writes a given word and definition to a file
 */
bool writeToFile(int number, std::string word, struct word w,
                 std::ofstream& file)
{
    bool success = true;

    std::stringstream ss;
    ss << number << ". " << word << " (" << w.wordType << ")" << ": "
       << w.wordDef << std::endl << std::endl;

    if (!file.is_open())
        success = false;
    
    else
        success = file << ss.str();

    return success;
}

int main(int argc, char *argv[])
{
    std::string buffer;
    char errorBuffer[CURL_ERROR_SIZE];

    word wordArray[argc - 1];

    std::ofstream file;
    file.open("definitions.txt");

    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();

    // grab the definitions from the words from the website
    for (int i = 1; i < argc; i++){
        std::string link = DIC_LINK;
        curlGetPage(link.append(argv[i]), buffer, errorBuffer, curl);

        word w = parse(buffer);
        wordArray[i - 1] = w;

        buffer.clear();
    }

    // write words and definitions to the file
    for (int i = 0; i < argc - 1; i++){
        if (!writeToFile(i + 1, argv[i + 1], wordArray[i], file)){
            std::cerr << "could not write word and definition to file";
        }
    }

    file << std::endl << "definition file generated by VocabQuiz Helper, created by Andrew Killian <Inceniium@gmail.com>";

    file.close();

    return 0;
}

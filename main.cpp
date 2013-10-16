#include <iostream>
#include <map>
#include <string>
#include <algorithm>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/algorithm/string.hpp>

/*! \mainpage This programm counts the repeted words.
 *  To compile the program and the follow use you need to install the boost library. 
 *
 *  The file make.sh is using to compile this programm.
 *  Use the following expressin to run the programm: ./urlcounter <URL>
 *  Result of the programm will be displayed in the file output.txt.
 *  Logs of the programm will be displayed in the file trace.txt.
 */


using boost::asio::ip::tcp;
namespace asio = boost::asio;
using namespace std;
std::ofstream trace;

//! Using the synchronous http client.
class HttpClient
{
public:
    //! It's the defualt constructor.
	/*!
		It's creating a socket that is attached to the service. 
	*/
    HttpClient() : socket(io_service)
    {}
    
    //! Reading contnent of the page.
	/*!
		There is a connection to host then sending request and after reading response.
		\param host - ip-adress or domain name.
		\return It's returning content of the page without header HTTP protocol.
	*/
    std::string ReadContent(const std::string& host)
    {
        Connect(host);
        
        if (SendRequest(host))
            return GetContentResponse();
        
        return "";
    }
    
private:
    //! Connection to host.
	/*!
		There is creating an integrator by points connection to host and open the socket.
		\param host - ip-adress or domain name.
	*/
    void Connect(const std::string& host)
    {
        try
        {
            if (host.empty())
                return;
            trace<<"Conecting to " << host <<"\n"<< endl;
            tcp::resolver resolver(io_service);
            tcp::resolver::query query(host, "http");
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        
            asio::connect(socket, endpoint_iterator);
        }
        catch (std::exception& e)
        {
            std::cout << "Exception: " << e.what() << "\n";
        }
        return;
    }
    
    //! Sending request.
	/*!
		A request with termination of connection after the server response received.
		\param host - ip-adress or domain name.
		\return It is return the value TRUE if the host name is not empty and unable to write request to the socket.
	*/
    bool SendRequest(const std::string& host)
    {
        try
        {
            if (host.empty())
                return false;
    	    trace<<"Sending request\n"<<endl;
            asio::streambuf request;
	    std::ostream request_stream(&request);
            request_stream << "GET / HTTP/1.0\r\n";
            request_stream << "Host: " << host << "\r\n";
            request_stream << "Accept: */*\r\n";
            request_stream << "Connection: close\r\n\r\n";
    	    trace<<"Write the socket"<< endl;
	    trace<<"Write the request "<< endl;
            boost::asio::write(socket, request);
            return true;
        }
        catch (std::exception& e)
        {
            std::cout << "Exception: " << e.what() << "\n";
        }
        return false;
    }
    
    //! Reading the answer and returning content of the page.
    /*!
		There is reading and handling responses from the server. After removes the http protocol header.
		\return It is return the page without http protocol header and return an empty string if an error occurs.
	*/
    std::string GetContentResponse()
    {
        try
        {
            asio::streambuf response;
            asio::read_until(socket, response, "\r\n");
            
            std::istream response_stream(&response);
            std::string http_version;
            response_stream >> http_version;
	    trace<<"The HTTP version is "<<http_version<<"\n"<<endl;
            unsigned int status_code;
            response_stream >> status_code;
	    trace<<"Status code is "<<status_code<<"\n"<<endl;
            std::string status_message;
            std::getline(response_stream, status_message);
            if (!response_stream || http_version.substr(0, 5) != "HTTP/")
            {
                std::cout << "Invalid server response format\n";
                return "";
            }
            if (status_code != 200)
            {
                std::cout << "The server returned an error code -" << status_code << "\n";
                return "";
            }
            
	    trace<<"The response header is reading and then passing.\n"<<endl;
            
			asio::read_until(socket, response, "\r\n\r\n");
            std::string header;
            while (std::getline(response_stream, header) && header != "\r")
            {}
            
            std::ostringstream stringbuf;
            boost::system::error_code error;
            while (boost::asio::read(socket, response,
                                     boost::asio::transfer_at_least(1), error))
            {
                stringbuf << &response;
            }
            if (error != boost::asio::error::eof)
                throw boost::system::system_error(error);
            
            return stringbuf.str();
        }
        catch (std::exception& e)
        {
            std::cout << "Exception: " << e.what() << "\n";
        }
        return "";
    }
    
private:
    //! Service
	asio::io_service io_service;
    //! Socket
	tcp::socket socket;
};


//! Data hendling and counts of the words from page.
/*!
	It is a class for hendling html page and counts of the words on it.
	There is delete HTML tag. Then there are counted the word, sorted by it popularity.
*/
class WordsCounter
{
	//! A hash-array: key is the word, value is counts of the words on that page.
	typedef std::map<std::string, int> string_map;
	
	//! The massif pairs of words is the number of repetition.
	typedef std::vector<std::pair<std::string, int>> string_vector;

public:
    //! There is returns massif of words and number of it repetition from html page.
	/*! 
		\param text - defualt text from html page.
		\return There is returning massif pairs of words and number of it repetition sorted by descending.
	*/
    string_vector GetWordsSortedByCount(const std::string& text)
    {
        string_map map = CreateWordsMap(RemoveHtmlTags(text));
        return SortMapByValue(map);
    }
    
private:
    //! From sourse code of the page removed all html tags.
    /*! 
		A tags like this one "<...>" are deleting.
		\param html - a defualt text from html page.
		\return There is returning text without tags.
	*/
    static std::string RemoveHtmlTags(const std::string& html)
    {
        std::string result;
        bool add=true;
        for(char c : html)
        {
            if (c=='<')
                add=false;
            if (add)
                result += c;
            if (c=='>') {
                add=true;
		result+=' ';
	    }
        }
        return result;
    
    }
    
    //! A hash-array of words is creating with repetition counter.
    /*! 
		As words separators are used next symbols: space , . : ; " ' ( ) [ ] { } ! ? / | \ -
		Also the word can be separated by tabs and new line symbols.
		\param text - text for the processing.
		\return A hash-array contain next description: a key is the word, a value is number of counts thea words in the text.
	*/
    static string_map CreateWordsMap(const std::string& text)
    {
        std::vector<std::string> words;
        boost::split(words, text, boost::is_any_of("\t\n ,.:;\"'()[]{}!?/|\\-"));
        string_map words_map;
        for(auto word : words)
            if (!word.empty())
                words_map[word]++;
        
        return words_map;
    }
    
    
    //! For sorting data are compares pairs "word - amount".
    static bool ComparePair(std::pair<std::string, int> p1, std::pair<std::string, int> p2)
    {
        return (p1.second>p2.second ||
                (p1.second==p2.second && p1.first<p2.first));
    }
    
    //! A words are sorting by the number of repetition.
    /*! 
		Containts of the hash-array is sorting in descending order of frequency of repetiton of words and lexicographical order.
		\param map - a hash-array contain next description: a key is the word, a value is the number of counts the word in the text.
		\return Then returns a sorted  massif pairs "word - amoun" in descending order.
	*/
    static string_vector SortMapByValue(const string_map& map)
    {
	string_vector words_vector;
        for(auto pair : map)
            words_vector.push_back(pair);
        
        std::sort(words_vector.begin(), words_vector.end(), ComparePair);
        
        return words_vector;
    }
};


int main(int argc, const char * argv[])
{
    std::string host("www.yandex.ru");
    if (argc > 1)
    {
        host = std::string(argv[1]);
        std::cout<<"Download content from \""<<host<<"\"...\n";
    }
    else
    {
        std::cout<<"Download content from \"www.yandex.ru\"...\n"
                 <<"Usage: "<<argv[0]<<" [host_name] for download from \"host_name\".\n";
    }
    
    trace.open ("trace.txt");
    HttpClient client;
    auto text_content = client.ReadContent(host);
    
    WordsCounter counter;
    auto words_counts = counter.GetWordsSortedByCount(text_content);
    
	std::ofstream file;
	file.open ("output.txt");
    for(auto pair : words_counts)
        file<< pair.first<<"\t"<<pair.second<<"\n";
	file.close();
    
    std::cout<<"Complete.\n";
    trace.close();

	return 0;
}


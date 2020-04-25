// Copyright 2018 Your Name <your_email>

#ifndef INCLUDE_HEADER_HPP_
#define INCLUDE_HEADER_HPP_

#include <iostream>
#include <vector>
#include <mutex>
#include <algorithm>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions.hpp>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

#define MAX_MSG 1024


//using namespace boost::asio;

using std::cout;
using std::endl;


boost::asio::io_service service; // создание экземпляра io_service для общения с сервисом ввода/вывода ОС
boost::recursive_mutex mx;

class Server;

// Вектор клиентов
std::vector<std::shared_ptr<Server>> clients; //нужен список клиентов, чтобы обрабатывать входящие запросы от них.

class Server {
private:
    boost::asio::ip::tcp::socket sock;
    int already_read_;
    char buff_[MAX_MSG];
    std::string username_;
    bool clients_changed_;
    boost::posix_time::ptime last_ping;

public:
    Server() : sock(service), clients_changed_(false) {}

    boost::asio::ip::tcp::socket &sock_r() {
        return sock;
    }

    std::string username() const {
        return username_;
    }

    void answer_to_client() {   // Ответ клиенту
        try {
            read_request();     // Чтение запроса //Чтение будет происходить только если есть данные, таким образом, сервер никогда не будет заблокирован
            process_request();  //Обработка запроса
        }
        // Обработка ошибки, которая может произойти в блоке try
        catch (boost::system::system_error &) {
            stop(); // Выключение сервера
        }

        // Провека на время
        // Если клиент не пингукется в теченнии 5 сек, то кикнуть его
        if (timed_out())
            stop();
    }

    void read_request() {   // Чтение запроса
        if (sock.available()) //если сокет доступен
            already_read_ += sock.read_some(boost::asio::buffer
                    (buff_ + already_read_, MAX_MSG - already_read_)); //в buff_ сохраняет запрос,который не больше 1024 символов
    }

    void process_request() {    // Обработка полученого запроса
        // После того как мы считали те данные, которые были доступны, мы должны проверить
        // считали ли мы сообщение до конца (если да, то found_enteris установится в true).
        // Если это так, то мы защищаем себя от чтения больше чем одного сообщения
        // (после символа ‘\n’ сохраняться в буфер ничего не будет),
        // а затем мы интерпретируем полностью прочитанное сообщение
        bool found_enter = std::find(buff_, buff_ + already_read_, '\n')
                < buff_ + already_read_; // проверка считали ли мы сообщение до конца
        if (!found_enter)
            return;

        // Метка для засекания пинга
        last_ping = boost::posix_time::microsec_clock::local_time();

        // Парсим сообщение
        size_t pos = std::find(buff_, buff_ + already_read_, '\n') - buff_;
        std::string msg(buff_, pos);
        std::copy(buff_ + already_read_, buff_ + MAX_MSG, buff_);
        already_read_ -= pos + 1;

        if (msg.find("login ") == 0) on_login(msg); //индекс с которого начинается подстрока  // Регистрация пользователя
        else if (msg.find("ping") == 0) on_ping();
        else if (msg.find("ask_clients") == 0) on_clients(); //ответ клиенту
        else
            std::cerr << "invalid msg " << msg << std::endl; //поток вывода ошибок
    }

    void on_login(const std::string &msg) {    // Регистрация пользователя
        std::istringstream in(msg); //stringstream позволяет связать поток ввода-вывода со строкой в памяти
        in >> username_ >> username_;
        //std::cout<< username_ <<std::endl;
        write("login ok\n"); // в сокет записывается выражение
        {
            // Потокобезопасный доступ к вектору клиентов
            boost::recursive_mutex::scoped_lock lk(mx); //чтобы потоки не могли одновременно работать с одними данными
            for (unsigned i = 0; i < clients.size() - 1; i++) // Каждый клиент выставляет флаг
                clients[i]->update_clients_changed();
        }
    }

    // Обновление списка клиентов (выставление флага)
    void update_clients_changed() {
        clients_changed_ = true;
    }

    // Клиент может делать следующие запросы: получить список
    // всех подключенных клиентов и пинговаться,
    // где в ответе сервера будет либо ping_ok,
    // либо client_list_chaned
    // (в последнем случае клиент повторно запрашивает список
    // подключенных клиентов);
    void on_ping() {
        //std::cout<<clients_changed_<<std::endl;
        write(clients_changed_ ? "ping client_list_changed\n" : "ping ok\n");//если список клиентов обновился, то выводится первое, если нет второе
        clients_changed_ = false;   // Сброс флага
    }

    void on_clients() { // Ответ клиенту
        std::string msg;
    {       // Потокобезопасный доступ к вектору клиентов
            boost::recursive_mutex::scoped_lock lk(mx); //чтобы потоки не могли одновременно работать с одними данными
            for (auto b = clients.begin(), e = clients.end(); b != e; ++b)
                msg += (*b)->username() + " ";
        }
        write("clients " + msg + "\n");
    }

    void write(const std::string &msg) {   // Запись сообшениия
        //cout<<msg<<endl;
        sock.write_some(boost::asio::buffer(msg));
    }

    void stop() {   // Закрытие сокета
        boost::system::error_code err;
        sock.close(err);
    }

    bool timed_out() const {    // Подсчет времени для отключения
        boost::posix_time::ptime now =
                boost::posix_time::microsec_clock::local_time();
        int ms = (now - last_ping).total_milliseconds();
        return ms > 5000;
    }
};

// Инициализация логов
static void init_logging() {
    
    logging::add_file_log (
                    keywords::file_name = "info.log",
                    keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%"
            );

    logging::add_console_log (
            std::cout,
            keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%"
    );

    logging::add_common_attributes();
}

// Поток для прослушивания новых клиентов
void accept_thread() {
    // Задаем порт (8001) для прослушивания и создаем акцептор (приемник)
    // — один объект (service), который принимает клиентские подключения
    boost::asio::ip::tcp::acceptor acceptor(service,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8001));
    init_logging(); // Инициализация логирования

    while (true) {
        // Создаем  умный указатель cl (на сокет)
        std::shared_ptr<Server> cl = std::make_shared<Server>();
        std::cout << "wait client" << std::endl;
        //Принять запрос на установку соединения accept
        acceptor.accept(cl->sock_r());  // инициализация сокета // Ждем подключение клиента
        BOOST_LOG_TRIVIAL(trace) << "client acepted" << endl;
        // Потокобезопасный доступ к вектору клиентов
        boost::recursive_mutex::scoped_lock lk(mx); //чтобы потоки не могли одновременно работать с одними данными
        clients.push_back(cl);  // Добавление нового клиента в вектор
    }
}

// Поток для прослушиваниия существующих клиентов
void handle_clients_thread() {
    while (true) {
        boost::this_thread::sleep(boost::posix_time::millisec(1));//очередь потоков
        // Потокобезопасный доступ к вектору клиентов
        boost::recursive_mutex::scoped_lock lk(mx);

        for (auto b = clients.begin(); b != clients.end(); ++b)
            (*b)->answer_to_client();   // Отпраляем ответы КАЖДОМУ клиенту
            //вызываем answer_to_client для каждого клиаента

        // Удаляем клиенты, у которых закончилось время
        clients.erase(std::remove_if(clients.begin(), clients.end(),
                    boost::bind(&Server::timed_out, _1)), clients.end());
    }
}


int main() {
    boost::thread_group threads;
    //т.к accept() блокирующая операция создаем 2 потока
    threads.create_thread(accept_thread); // Поток для прослушивания новых клиентов
    threads.create_thread(handle_clients_thread);// Поток для обработки существующих клиентов
    // Запуск потоков и ожидание завершения последнего
    threads.join_all();
    return 0;
}
#endif // INCLUDE_HEADER_HPP_

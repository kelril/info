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

//  mutex - это взамное исключение, базовый механизм синхранизации.
// Предназначен для организации взаиносключающего доступа к общим данным
// для нескольких потоков с использованием барьеров памяти

//  Синронная модель - потоку назначается одна задача и начинается выполнение.
// Когда завершено выполнение появляется возможность заниматься другой задачей
// МИНУСЫ: невозможно остановить выполнение задачи, чтобы в промежутке выполнить другую задачу.

//  Асинхронная модель - в отличии от синхр. модели, тут поток может приостоновиться,
// и выполнить другую задачу, а потом вернуться к предыдущей.

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

#define MAX_MSG 1024

using std::cout;
using std::endl;

// Создаем экземпляр для общения с сервисом ввода/вывода ОС
boost::asio::io_service service;
boost::recursive_mutex mx;

class Server;

std::vector<std::shared_ptr<Server>> clients; // Вектор клиентов

class Server
{
private:
    boost::asio::ip::tcp::socket sock;
    int already_read_;
    char buff_[MAX_MSG];
    std::string username_;
    bool clients_changed_;
    boost::posix_time::ptime last_ping;

public:
    Server() : sock(service), clients_changed_(false) {}

    boost::asio::ip::tcp::socket &sock_r()
    {
        return sock; //ретёрн сокета 219 строка. Занесение сокета в класс
    }

    std::string username() const
    {
        return username_;
    }

    void answer_to_client() // Ответ клиенту
    {
        try
        {
            read_request();    // Чтение запроса
            process_request(); // Обработка запроса
        }

        catch (boost::system::system_error &) // Обработка ошибки, которая может произойти в блоке try
        {
            stop(); // Выключение сервера
        }

        if (timed_out()) // Провека на время
            stop();      // Если клиент не пингукется в теченнии 5 сек, то кикнуть его
    }

    void read_request()
    { // Чтение запроса в том случае, когда есть данные.
        if (sock.available())
            already_read_ += sock.read_some(boost::asio::buffer(buff_ + already_read_, MAX_MSG - already_read_));
    }

    void process_request()
    {   // Обработка полученого запроса: (1) защита от чтения больше 1-го сообщения,
        // (2) проверка считали ли мы до конца сообщение
        bool found_enter = std::find(buff_, buff_ + already_read_, '\n') < buff_ + already_read_;
        if (!found_enter)
            return;

        // Метка для засекания пинга
        last_ping = boost::posix_time::microsec_clock::local_time();

        // Парсим сообщение
        size_t pos = std::find(buff_, buff_ + already_read_, '\n') - buff_;
        std::string msg(buff_, pos);
        std::copy(buff_ + already_read_, buff_ + MAX_MSG, buff_);
        already_read_ -= pos + 1;

        if (msg.find("login ") == 0)
            on_login(msg); // Метод поиска подроки в строке(login). На нулевом месте.
        else if (msg.find("ping") == 0)
            on_ping();
        else if (msg.find("ask_clients") == 0)
            on_clients();
        else
            std::cerr << "invalid msg " << msg << std::endl; // Поток вывода ошибок
    }

    void on_login(const std::string &msg)
    {                                 // Регистрация пользователя
        std::istringstream in(msg);   // Связывание потока сос трокой
        in >> username_ >> username_; // занос в приват
        write("login ok\n");
        {
            boost::recursive_mutex::scoped_lock lk(mx);
            // Каждый клиент выставляет флаг
            for (unsigned i = 0; i < clients.size() - 1; i++)
                clients[i]->update_clients_changed(); // установка флага в векторе единицой.
        }
    }

    // Обновление списка клиентов (выставление флага)
    void update_clients_changed()
    {
        clients_changed_ = true;
    }

    // Клиент может делать следующие запросы: получить список
    // всех подключенных клиентов и пинговаться,
    // где в ответе сервера будет либо ping_ok,
    // либо client_list_chaned
    // (в последнем случае клиент повторно запрашивает список
    // подключенных клиентов);
    void on_ping()
    {
        write(clients_changed_ ? "ping client_list_changed\n" : "ping ok\n"); //true or falls
        clients_changed_ = false;                                             // Сброс флага
    }

    void on_clients() // Ответ клиенту
    {
        std::string msg;
        {
            boost::recursive_mutex::scoped_lock lk(mx);
            for (auto b = clients.begin(), e = clients.end(); b != e; ++b)
                msg += (*b)->username() + " ";
        }
        write("clients " + msg + "\n");
    }

    void write(const std::string &msg)
    { // Запись сообшениия
        //cout<<msg<<endl;
        sock.write_some(boost::asio::buffer(msg));
    }

    void stop() // Закрытие сокета
    {
        boost::system::error_code err;
        sock.close(err);
    }

    bool timed_out() const // Подсчет времени для отключения
    {
        boost::posix_time::ptime now =
            boost::posix_time::microsec_clock::local_time();
        int ms = (now - last_ping).total_milliseconds();
        return ms > 5000;
    }
};

static void init_logging() // Инициализация логов
{
    logging::add_file_log(
        keywords::file_name = "info.log",
        keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%");

    logging::add_console_log(
        std::cout,
        keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%");

    logging::add_common_attributes();
}

void accept_thread() // Поток для прослушивания новых клиентов
{
    // Задаем порт для прослушивания и создаем акцептор (приемник)
    // — один объект, который принимает клиентские подключения
    boost::asio::ip::tcp::acceptor acceptor(service,
                                            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8001));
    init_logging();

    while (true)
    {
        std::shared_ptr<Server> cl = std::make_shared<Server>(); // Создаем  умный указатель cl (на сокет)
        std::cout << "wait client" << std::endl;
        acceptor.accept(cl->sock_r()); // Ждем подключение клиента. Обращаемся к полю и заносим класс в сокет. Вызов функции для подключение. геттер
        BOOST_LOG_TRIVIAL(trace) << "client acepted" << endl;
        // Потокобезопасный доступ к вектору клииентов
        boost::recursive_mutex::scoped_lock lk(mx); //взаимоисключающий доступ, для организации потоков.
        clients.push_back(cl);                      // Добавление нового клиента в вектор. добовление в конец вектора указателя
    }
}

// Поток для прослушиваниия существующих клиентов
void handle_clients_thread()
{
    while (true)
    {
        boost::this_thread::sleep(boost::posix_time::millisec(1)); // Потокобезопасный доступ к вектору клииентов
        boost::recursive_mutex::scoped_lock lk(mx);                // Очередь потоков

        for (auto b = clients.begin(); b != clients.end(); ++b)
            (*b)->answer_to_client(); // Отпраляем ответы КАЖДОМУ клиенту

        // Удаляем клиенты, у которых закончилось время
        clients.erase(std::remove_if(clients.begin(), clients.end(),
                                     boost::bind(&Server::timed_out, _1)),
                      clients.end());
    }
}

int main()
{
    boost::thread_group threads;
    // Поток для прослушивания новых клиентов
    threads.create_thread(accept_thread);
    // Поток для обработки существующих клиентов
    threads.create_thread(handle_clients_thread);
    // Запуск потоков и ожидание завершения последнего
    threads.join_all();
    return 0;
}
#endif // INCLUDE_HEADER_HPP_

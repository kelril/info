cmake_minimum_required(VERSION 3.4) #Версия CMake

set(CMAKE_CXX_STANDARD 17) #Стандарт с++
set(CMAKE_CXX_STANDARD_REQUIRED ON) #Обязательность установки указанного стандарта

option(BUILD_COVERAGE "Build coverage" OFF) #Прописываются возможные опции сборки(могут отсутствовать)
#Название опции, описание и значение по умолчанию
set(
        HUNTER_CACHE_SERVERS
        "https://github.com/bmstu-iu8-cpp-sem-3/hunter-cache"
        CACHE STRING "Default cache server"
)
#Прописывается настройки пакетного менеджера Hunter
include("tools/gate/cmake/HunterGate.cmake") #подключение CMake скрипта с доп. функциями

HunterGate(
        URL "https://github.com/cpp-pm/hunter/archive/v0.23.218.tar.gz" #URL к ядру Hanter
        SHA1 "9a3594a61227967fb92159339ba326701b287b19" #SHA1 хэш
)


# TODO: rename project and delete this comment
project(template) #Название проекта
string(APPEND CMAKE_CXX_FLAGS " -pedantic -Werror -Wall -Wextra") #Дополнительные опции для компилятора
string(APPEND CMAKE_CXX_FLAGS " -Wno-unused-command-line-argument")
string(APPEND CMAKE_CXX_FLAGS " -Wshadow -Wnon-virtual-dtor")

hunter_add_package(GTest) #Указание хантеру о добавлении пакета GTest(гугл тесты)
find_package(GTest CONFIG REQUIRED) #Указываем какие пакеты будут использоваться:(подключаться)
#CONFIG-ключевое слово, показывающая маску названий конфигурационных файлов, REQUIRED-обязательность подключения пакета(иначе-ошибка)
hunter_add_package(Boost #Указание хантеру о добавлении данных пакетов
        COMPONENTS
        program_options
        thread
        log
        filesystem)
find_package(Boost  #Указываем какие пакеты будут использоваться(подключаться)
        CONFIG REQUIRED
        program_options
        thread
        log
        log_setup
        filesystem)

hunter_add_package(rocksdb) #Указание хантеру о добавлении пакета rocksdb для работы с бд
find_package(RocksDB CONFIG REQUIRED) #подключение RocksDB
find_package(Threads) #подключение библиотеки (пакета) потоков Threads

#PUBLIC означает, что зависящему от библиотеки проекту тоже нужны эти опции
#PRIVATE означает, что опции нужны лишь для сборки библиотеки
#INTERFACE означает, что опции не нужны для сборки, но нужны для использования библиотеки
add_library(logs INTERFACE #говорим целям(logs), зависящим от библиотеки, что опции не нужны для сборки, но нужны для использования библиотеки
        )
add_library(header INTERFACE
        )

# TODO: change template word to project name and delete this comment
add_library(template STATIC #template-название проекта, STATIC-тип библиотеки(SHARED или STATIC).
        #SHARED-уже скомпелированная библиотека, а STATIC-библиотека, которая компелируется и линкуется во время работы программы
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/DBHashCreator.cpp #Путь к исходному каталогу, который в настоящее время обрабатывается
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/logs.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/main.cpp
        )
add_executable(tests #Цель для теста исполняемого файла
        # TODO: enum your files and delete this comment
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cpp
        )

# Указание директорий с заголовочными файлами: #PUBLIC означает, что зависящему от библиотеки проекту тоже нужны эти опции
#PRIVATE означает, что опции нужны лишь для сборки библиотеки
#INTERFACE означает, что опции не нужны для сборки, но нужны для использования библиотеки
target_include_directories(template
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )
target_include_directories(tests
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

# Указание библиотек для линковки
target_link_libraries(tests GTest::main template)

# Указание директорий с заголовочными файлами:
target_include_directories(
        header
        INTERFACE $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_include_directories(
        logs
        INTERFACE $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Указание библиотек для линковки
target_link_libraries(template Boost::log Boost::thread Boost::program_options Boost::log_setup Boost::filesystem
        RocksDB::rocksdb)   #подключение RocksDB
target_link_libraries(header INTERFACE Boost::log Boost::thread Boost::program_options Boost::log_setup Boost::filesystem
        RocksDB::rocksdb)    #подключение RocksDB

enable_testing() #включение тестирования
add_test(NAME unit_tests COMMAND tests) #при тестировании выполнится исполняемый файл

if(BUILD_COVERAGE) # Если активна опция сборки с покрытием
  set(ENABLE_COVERAGE ON CACHE BOOL "Enable coverage build." FORCE)  # Установка значения переменной ENABLE_COVERAGE
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")  # Дополнение возможных местоположений конфигурационных cmake файлов
  find_package(codecov) # Нахождение пакета (ожидается существование файла cmake/FindCodecov.cmake)
  # Далее прописываются цели, которые будут проанализированы на процент покрытия.
  add_coverage(template)
  add_coverage(tests)

  list(APPEND LCOV_REMOVE_PATTERNS "'${PROJECT_SOURCE_DIR}/tests/*'") # Удаление файлов с тестами из анализа
  coverage_evaluate() # Вызов анализатора
endif()
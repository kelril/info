cmake_minimum_required(VERSION 3.4) #Версия CMake

set(CMAKE_CXX_STANDARD 17) #Стандарт с++
set(CMAKE_CXX_STANDARD_REQUIRED ON) #Обязательность установки указанного стандарта

option(BUILD_COVERAGE "Build coverage" OFF)#Прописываются возможные опции сборки(могут отсутствовать)
#Название опции, описание и значение по умолчанию

set( #Прописывается настройки пакетного менеджера Hunter
  HUNTER_CACHE_SERVERS
  "https://github.com/bmstu-iu8-cpp-sem-3/hunter-cache"
  CACHE STRING "Default cache server"
)

include("cmake/HunterGate.cmake") #подключение CMake скрипта с доп. функциями

HunterGate(
  URL "https://github.com/cpp-pm/hunter/archive/v0.23.218.tar.gz" #URL к ядру Hanter
  SHA1 "9a3594a61227967fb92159339ba326701b287b19" #SHA1 хэш
)

project(builder)#Название проекта

hunter_add_package(Boost COMPONENTS program_options filesystem system regex) #Указание хантеру о добавлении данных пакетов
hunter_add_package(Async++) #Указание хантеру о добавлении пакета Async++, для работы с  синхронными и асинхронными процессами
#hunter_add_package(BoostProcess)
find_package(Async++ CONFIG REQUIRED) #Указываем какие пакеты будут использоваться(подключаться)
#find_package(BoostProcess CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED program_options filesystem system regex) #Указываем какие пакеты будут использоваться(подключаться)

add_library(builder STATIC #builder-название проекта, STATIC-тип библиотеки(SHARED или STATIC).
        #SHARED-уже скомпелированная библиотека, а STATIC-библиотека, которая компелируется и линкуется во время работы программы
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/builder.cpp #Путь к исходному каталогу, который в настоящее время обрабатывается
)
#PUBLIC означает, что зависящему от библиотеки проекту тоже нужны эти опции
#PRIVATE означает, что опции нужны лишь для сборки библиотеки
#INTERFACE означает, что опции не нужны для сборки, но нужны для использования библиотеки
add_library(process INTERFACE #говорим целям(process), зависящим от библиотеки, что опции не нужны для сборки, но нужны для использования библиотеки
)
add_executable(main  #Цель для main исполняемого файла
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/main.cpp
)

#PUBLIC означает, что зависящему от библиотеки проекту тоже нужны эти опции
#PRIVATE означает, что опции нужны лишь для сборки библиотеки
#INTERFACE означает, что опции не нужны для сборки, но нужны для использования библиотеки
target_include_directories(process  # Указание директорий с заголовочными файлами:
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/process/include
)
target_include_directories(builder  # Указание директорий с заголовочными файлами:
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Указание библиотек для линковки (сборки проекта)
target_link_libraries(builder process Async++::Async++
            Boost::program_options Boost::filesystem Boost::system Boost::regex)

target_link_libraries(main builder)

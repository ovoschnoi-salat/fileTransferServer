# fileTransferServer

## Запуск кода

### Сервер

`make server` - компиляция сервера  
`./server [-port <port>] [-path <path>]` - запуск сервера (для остановки сервера введите Ctrl+C)

### Клиент

`make client` - компиляция клиента  
`./client` - запуск клиента

### другое

`make clean` - удаляет временную папку с файлами  
`make clean-all` - удаляет временную папку и приложения client, server
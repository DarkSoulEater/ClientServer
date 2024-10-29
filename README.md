# ClientServer
Простое клиент-серверное приложение поддерживающее общение между сервером и клиентом. Реализовано на сокетах, поддерживает общение по протоколам UDP и TCP.

## Build
```cmd
make # Собрать проект
```

```
bin/Server -P [TCP/UDP] -p [port] # Запустить сревер
# Example: bin/Server -P TCP -p 10000
```

```
bin/Client -P [TCP/UDP] -p [port] # Запустить клиент
# Example: bin/Client -P TCP -p 10000
```
![](./Doc/Img/client_help.png)
![](./Doc/Img/server_help.png)
## Client
Клиент умеет только отправлять сообщения серверу.
![](./Doc/Img/client_start.png)
![](./Doc/Img/start_client_help.png)

Клиент отправил сообщение.

![](./Doc/Img/client_send.png)

## Server

Сервер поддерживает работу обмена сообщениями с несколькими клиентами одновременно в режиме TCP или UDP.

![](./Doc/Img/server_started.png)

Подключение клиента.

![](./Doc/Img/client_connect.png)

Клиент отправил сообщение.

![](./Doc/Img/client_send_server.png)

Сервер ответил.

![](./Doc/Img/server_recieve.png)

Небольшой диалог.

![](./Doc/Img/small_talk.png)

Можно посмотреть историю сообщение между сервером и клиентом.

![](./Doc/Img/dialog.png)

Клиент отключился.

![](./Doc/Img/disconected.png)

Можно посмотреть список ID подключенных клиентов.

![](./Doc/Img/clients.png)

Можно посмотреть список ID отключенных клиентов.

![](./Doc/Img/history.png)

Все команды сервера.

![](./Doc/Img/server_all_command.png)

Так же, тот же функционал сервер поддерживает в режиме UDP подключенний. Отключение UDP клиента происхоидт по таймауту. В режиме UDP, если к серверу придет сообщение от клиента, которого он отключил, он вспомнит его и не будет создавать новый ID.

## Console
Консоль обрабатывается вручную, благодаря чему неважно когда приходят сообщения, вывод будет правильный.

## Dump
В дампах снято создание сервера, отправка от клиента сообщения "Hello Server", отправка от сервера сообщения "Hello client", отключение клиента, закрытие сервера. (В UDP по два пакета на сообщение потому что я сначала отправляю размер сообщения и потом само сообщение).
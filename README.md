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
![](./Doc/client_help.png)
![](./Doc/server_help.png)
## Client
Клиент умеет только отправлять сообщения серверу.
![](./Doc/client_start.png)
![](./Doc/start_client_help.png)

Клиент отправил сообщение.

![](./Doc/client_send.png)

## Server

Сервер поддерживает работу обмена сообщениями с несколькими клиентами одновременно в режиме TCP или UDP.

![](./Doc/server_started.png)

Подключение клиента.

![](./Doc/client_connect.png)

Клиент отправил сообщение.

![](./Doc/client_send_server.png)

Сервер ответил.

![](./Doc/server_recieve.png)

Небольшой диалог.

![](./Doc/small_talk.png)

Можно посмотреть историю сообщение между сервером и клиентом.

![](./Doc/dialog.png)

Клиент отключился.

![](./Doc/disconected.png)

Можно посмотреть список ID подключенных клиентов.

![](./Doc/clients.png)

Можно посмотреть список ID отключенных клиентов.

![](./Doc/history.png)

Все команды сервера.

![](./Doc/server_all_command.png)

Так же, тот же функционал сервер поддерживает в режиме UDP подключенний. Отключение UDP клиента происхоидт по таймауту. В режиме UDP, если к серверу придет сообщение от клиента, которого он отключил, он вспомнит его и не будет создавать новый ID.
# Описание.
Данная программа передает файл с Пк1 на Пк2, оба компьютера могут использовать как Windows, так и Linux.
Файл передается кусочками по 4Кб несколькими потоками.
# Как пользоваться.
1. Запустите программу с флагом "recieve" на компьютере, который будет принимать файл. Пример: "program -recieve".
2. На другом компьютере запускаете программу в формате "program -send -ip <IP> -file <FILENAME> -threads <THREAD>".
После -ip идет адрес принимающего компьютера в сети, после -file идет полный путь до файла, после -threads кол-во потоков,
которое вы хотите использовать.
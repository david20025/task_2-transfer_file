# Описание.

Данная программа передает файл с Пк1 на Пк2, оба компьютера могут использовать как Windows, так и Linux.
Файл передается кусочками по 4Кб несколькими потоками.
Вы можете задать кол-во потоков самостоятельно через `-threads NUMBER_THREADS`.
# Как пользоваться.

+ Запустите программу с флагом `recieve` на компьютере, который будет принимать файл. Пример: `program -recieve`.
+ На другом компьютере запускаете программу в формате `program -send -ip IP -file FILENAME -threads NUMBER_THREADS`.
* `-ip` - адрес принимающего компьтера в сети.
* `-file` - полный путь до файла.
* `-threads` - кол-во поток, которое нужно использовать для передачи.

# Discription.

This program transfers the file from PC1 to PC2, both computers can use both Windows and Linux.
The file is transmitted in 4KB chunks by several threads.
You can set the number of threads yourself using `-threads NUMBER_THREADS`.

# How to use.

+ Run the program with the `receive` flag on the computer that will receive the file. Example: `program -receive`.
+ On another computer, run the program in the format `program -send -ip IP -file FILENAME -threads NUMBER_THREADS`.
* `-ip` - IP address of the receiving computer on the network.
* `-file` - full path to the file.
* `-threads` - number of threads to be used for transmission.


# CSV WaveForm

CSV WaveForm - грфическая утилита для визуализации временных диаграмм,
заключенных в CSV - подобных файлах (одна строчка: время и значения,
разделённые запятыми/табуляциями/др..).

Программа позволяет разглядывать временные диаграммы, записанные осциллографом,
сгенерированные Matlab/Octave/C++/Verilog/Ngspice, выдернутые из SQL/mp3/wav и др. 

Аналогичные функции выполняют программы Gwave и GAW (под Linux).

Преимущество CSV WaveForm перед выше перечисленными программами состоит в
1) простоте сборки (под Linux не пробовал, но проблем там не более и не менее,
чем со сборкой любой программы QT 5.6.3);
2) возможности подстроиться под экзотический тип файла (без перекомпиляции проекта), дополнив файл /etc/loadfile.lua (язык Lua 5.3 без расширений).

Программа будет полезна студентам технических ВУЗов и тем кто пишет для 
этих студентов методические пособия...


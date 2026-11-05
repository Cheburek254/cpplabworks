# cpplabworks

Решения лабораторных работ и контрольных livecoding по курсу C++ ИТМО за 2025/26 учебный год. В каждой папке есть исходники, материалы из выбранной ветки и README с заданием.

## Работы

| Папка | Тема | Стандарт | Ветка и исходный коммит |
| --- | --- | --- | --- |
| [labwork1](labwork1/) | Шаблонизатор текстовых документов | C++23 | `dd0_501430` · [`828ab6e`](https://github.com/is-itmo-c-25/labwork1-Cheburek254/commit/828ab6e563aa785eb020ec9fe4842b1c82d9fb4f) |
| [labwork2](labwork2/) | Знаковое целое `int2025_t` | C++23 | `deadline_1` · [`ed6bcd2`](https://github.com/is-itmo-c-25/labwork2-Cheburek254/commit/ed6bcd230c10c9470661b95e1dd5fb56355c4828) |
| [labwork3](labwork3/) | Библиотека разбора аргументов командной строки | C++23 | `dedline_1` · [`aead481`](https://github.com/is-itmo-c-25/labwork3-Cheburek254/commit/aead4814d3f97e912b1734ea1166c6664b625b93) |
| [labwork4](labwork4/) | Архиватор HamArc с кодами Хэмминга | C++23 | `deadline_3` · [`ed1329c`](https://github.com/is-itmo-c-25/labwork4-Cheburek254/commit/ed1329c63fab3c0f936373f8feb965b13262ee1c) |
| [labwork5](labwork5/) | ITMO Loops, синтез WAV из текстовой партитуры | C++20 | `deadline_3` · [`0afd0ca`](https://github.com/is-itmo-c-25/labwork5-Cheburek254/commit/0afd0cad8b57cdf3b08e6fd5865185a7b132d9c3) |
| [labwork6](labwork6/) | STL-совместимый циклический буфер | C++23 | `deadline_0` · [`d407988`](https://github.com/is-itmo-c-25/labwork6-Cheburek254/commit/d407988abfe1451582776a8b67a03eb1bffbd62b) |
| [labwork7](labwork7/) | Поиск маршрутов через API Яндекс Расписаний с кэшированием | C++23 | `deadline_0` · [`123e085`](https://github.com/is-itmo-c-25/labwork7-Cheburek254/commit/123e085146612c0dd1a4d4ab327b61af26bc17ad) |
| [labwork8](labwork8/) | Адаптеры для конвейерной обработки данных | C++23 | `deadline_0` · [`22fbc0f`](https://github.com/is-itmo-c-25/labwork8-Cheburek254/commit/22fbc0fc8ddb80c937b6f595f9f25b64321cae4c) |
| [labwork9](labwork9/) | Планировщик задач и рекомендации мест по погоде | C++17 | `deadline_0` · [`dca42cb`](https://github.com/is-itmo-c-25/labwork9-Cheburek254/commit/dca42cb18448da81e7b32e4d8c4ae07d5d72c6bf) |
| [labwork10](labwork10/) | Локальная in-memory key-value база данных | C++20 | `deadline_0` · [`84b8a00`](https://github.com/is-itmo-c-25/labwork10-Cheburek254/commit/84b8a009d0b8b8857d6909768c743b6455726231) |
| [livecoding](livecoding/) | Консольная игра Wordle | C++23 | `livecoding` · [`1302597`](https://github.com/is-itmo-c-25/livecoding-Cheburek254/commit/1302597180d343b9d9643a0d87b87fa4605f967d) |
| [livecoding2](livecoding2/) | Алгоритм `IsTwiceSequence` для диапазона итераторов | C++23 | `livecoding` · [`611664c`](https://github.com/is-itmo-c-25/livecoding2-Cheburek254/commit/611664cbf53bcbd0e426e92d61e80bf2858be374) |

Для лабораторных выбрана ветка с наибольшим номером дедлайна. Первая работа перенесена из ветки `dd0_501430`, а обе контрольные — из веток `livecoding`.

## Сборка

Каждая работа собирается отдельно. Например, для лабораторной № 2 выполните из корня репозитория:

```sh
cmake -S labwork2 -B labwork2/build
cmake --build labwork2/build --config Release
ctest --test-dir labwork2/build -C Release --output-on-failure
```

Для другой работы подставьте имя её папки. Используйте компилятор с поддержкой стандарта из таблицы. Если у работы есть тесты, их можно запустить через CTest. При конфигурации некоторых работ CMake загружает внешние зависимости.

## Дискретная математика

В [discrete math](discrete%20math/) лежат домашние задания по кодам Хэмминга и графам.

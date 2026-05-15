/***********************************************************************************************************************
*   Flash память МК серии N32G45x
*       Регистры, битовые маски, адреса
***********************************************************************************************************************/

#ifndef __N32G45x_H__
#define __N32G45x_H__


/** Данные о Flash памяти N32G45x */
#define FLASH_ADDRESS_START         0x08000000u         // Начальный адрес Flash памяти
#define FLASH_ADDRESS_END           0x0807FFFFu         // Конечный адрес Flash памяти
#define FLASH_PAGE_SIZE             0x800u              // Размер страницы Flash памяти
#define FLASH_PAGE_COUNT            256                 // Количество страниц Flash памяти

/** Адреса 32-битных регистров FLASH */
#define FLASH_AC        0x40022000u     /** FLASH access control register */
#define FLASH_KEY       0x40022004u     /** FLASH key register */
#define FLASH_OPTKEY    0x40022008u     /** FLASH OPTKEY register */
#define FLASH_STS       0x4002200Cu     /** FLASH status register */
#define FLASH_CTRL      0x40022010u     /** FLASH control register */
#define FLASH_ADD       0x40022014u     /** FLASH address register */
#define FLASH_OB        0x4002201Cu     /** FLASH Option byte register */
#define FLASH_WPR       0x40022020u     /** FLASH Write protection register */
#define FLASH_ECC       0x40022024u     /** FLASH ECC register */
#define FLASH_RDN       0x4002202Cu     /** FLASH RDN register */
#define FLASH_CAHR      0x40022030u     /** FLASH CAHR register */


/** Unlock Flash (записывается в FLASH_KEY) */
#define KEY1            0x45670123
#define KEY2            0xCDEF89AB

/** Unlock Option Bytes (записывается в FLASH_OPTKEY) */
#define OPTKEY1         0x45670123
#define OPTKEY2         0xCDEF89AB


/** Биты регистра FLASH_STS */
#define FLASH_STS_EECERR    (0x1 << 7)      /** ECC error                       */  // Ошибка чтения Flash, аппаратно устанавливается 1, записать программно 1 для сброса бита
#define FLASH_STS_EVERR     (0x1 << 6)      /** Erase check error               */  // Страница стерта, проверка выдала ошибку, аппаратно устанавливается 1, записать программно 1 для сброса бита
#define FLASH_STS_EOP       (0x1 << 5)      /** End of operation                */  // Аппаратно устанавливается 1 по завершении операции (программирования/стирания), записать программно 1 для сброса бита
#define FLASH_STS_WRPERR    (0x1 << 4)      /** Write protection error          */  // Аппаратно устанавливается 1 при попытке записать по защищенному от записи адресу, записать программно 1 для сброса бита
#define FLASH_STS_PVERR     (0x1 << 3)      /** Programming verification error  */  // Аппаратно устанавливается 1 при ошибке верификации записанной памяти, записать программно 1 для сброса бита
#define FLASH_STS_PGERR     (0x1 << 2)      /** Programming error               */  // Аппаратно устанавливается 1 при ошибке записи, записать программно 1 для сброса бита
#define FLASH_STS_BUSY      (0x1 << 0)      /** Busy                            */  // Аппаратно устанавливается 1 при начале операции, аппаратно сбрасывается 1 после завершения операции или в случае ошибки


/** Биты регистра FLASH_CTRL */
#define FLASH_CTRL_ECERRITE (0x1 << 13) /** ECC error interrupt                     */  // 1 - разрешена (0 - запрещена) генерация прерываний при поднятии флага FLASH_STS_EECERR
#define FLASH_CTRL_EOPITE   (0x1 << 12) /** Allow operation completion interrupt    */  // 1 - разрешена (0 - запрещена) генерация прерываний при поднятии флага FLASH_STS_EOP
#define FLASH_CTRL_FERRITE  (0x1 << 11) /** Erase/Program Verify Error Interrupt    */  // 1 - разрешена (0 - запрещена) генерация прерываний при поднятии флагов FLASH_STS_PVERR или FLASH_STS_PGERR
#define FLASH_CTRL_ERRITE   (0x1 << 10) /** Error status interrupt allowed          */  // 1 - разрешена (0 - запрещена) генерация прерываний при ошибках Flash
#define FLASH_CTRL_OPTWE    (0x1 << 9)  /** Allow write option byte                 */  // 0 - запрещена запись в Option bytes, 1 устанавливается после записи разблокирующей последовательности в FLASH_OPTKEY
#define FLASH_CTRL_SMPSEL   (0x1 << 8)  /** Flash programming mode options          */  // 0 - перед записью в память необходимо прочитать содержимое, если память не была стерта, установится бит FLASH_STS_PGERR
                                                                                        // 1 - нет проверки стертости памяти перед записью, но если в этой памяти уже записаны данные, поверх них можно корректно записать только то же самое
#define FLASH_CTRL_LOCK     (0x1 << 7)  /** Lock                                    */  // 1 - Flash и FLASH_CTRL заблокированы, после корректной разблокирующей последовательности в FLASH_KEY аппаратно становится 0
#define FLASH_CTRL_START    (0x1 << 6)  /** Start                                   */  // 1 - запуск операции стирания, 0 устанавливается только когда FLASH_STS_BUSY становится 1
#define FLASH_CTRL_OPTER    (0x1 << 5)  /** Erase option bytes                      */  // 0 - отключить режим стирания Option bytes, 1 - включить
#define FLASH_CTRL_OPTPG    (0x1 << 4)  /** Program option bytes                    */  // 0 - отключить режим программирования Option bytes, 1 - включить
#define FLASH_CTRL_MER      (0x1 << 2)  /** Mass erase                              */  // 0 - отключить режим полного стирания, 1 - включить
#define FLASH_CTRL_PER      (0x1 << 1)  /** Page erase                              */  // 0 - отключить режим стирания страницы, 1 - включить
#define FLASH_CTRL_PG       (0x1 << 0)  /** Program                                 */  // 0 - отключить режим программирования, 1 - включить


/** Биты регистра FLASH_OB */
#define FLASH_OB_RDPRT2     (0x1 << 31) /** Read protection L2 level protection         */  // RO 0 - запрещено, 1 - разрешено
#define FLASH_OB_nRST_STDBY (0x1 << 4)  /** Enter Standby mode reset configuration      */  // RO 0 - сброс после вхождения в STANDBY, 1 - без сброса
#define FLASH_OB_nRST_STOP  (0x1 << 3)  /** Enter STOP0/STOP2 mode reset configuration  */  // RO 0 - сброс после вхождения в STOP0/STOP2, 1 - без сброса
#define FLASH_OB_WDG_SW     (0x1 << 2)  /** Set watchdog                                */  // RO 0 - аппаратный вочдог, 1 - программный вочдог
#define FLASH_OB_RDPRT1     (0x1 << 1)  /** Read protection L1 level protection         */  // RO 0 - запрещено, 1 - разрешено
#define FLASH_OB_OBERR      (0x1 << 0)  /** Option byte error                           */  // RO 1 - option byte не соответствует своему дополнению



/** Разблокировать/заблокировать Flash
*       После сброса модуль Flash защищен, поэтому невозможно записывать в регистр FLASH_CTRL и в память.
*       Записывается сначала KEY1, затем KEY2 в регистр FLASH_KEY, после этого бит FLASH_CTRL_LOCK должен сброситься.
*   Для блокировки Flash достаточно программно записать в FLASH_CTRL_LOCK 1. */

/** Стирание Flash памяти
*       Стирание 1 страницы:
*   Убедиться, что FLASH_STS_BUSY == 0 (не выполняется никакая другая операция стирания/записи).
*   FLASH_CTRL_PER = 1.
*   Выбрать стираемую страницу (в регистр FLASH_ADD записать адрес страницы).
*   FLASH_CTRL_START = 1.
*   Подождать пока FLASH_STS_BUSY не станет 0.
*   Прочитать стертую память, убедиться что она действительно стерта.
*
*       Полное стирание:
*   Убедиться, что FLASH_STS_BUSY == 0 (не выполняется никакая другая операция стирания/записи).
*   FLASH_CTRL_MER = 1.
*   FLASH_CTRL_START = 1.
*   Подождать пока FLASH_STS_BUSY не станет 0.
*   Прочитать стертую память, убедиться что она действительно стерта.
*/

/** Запись в основную область памяти
*       Запись происходит после стирания памяти.
*       Убедиться, что FLASH_STS_BUSY == 0 (не выполняется никакая другая операция стирания/записи).
*       FLASH_CTRL_PG = 1.
*       Записать 32-битное слово данных по нужному адресу как *address_in_flash = value;
*       Подождать пока FLASH_STS_BUSY не станет 0.
*       Прочитать записанную память, убедиться что она записана правильно.
*/

/** Option bytes запись и стирание
*       Стирание
*   Убедиться, что FLASH_STS_BUSY == 0 (не выполняется никакая другая операция стирания/записи).
*   Разблокировать FLASH_CTRL_OPTWE
*   FLASH_CTRL_OPTER = 1.
*   FLASH_CTRL_START = 1.
*   Подождать пока FLASH_STS_BUSY не станет 0.
*   Прочитать стертый Option byte, убедиться что он действительно стерт.
*
*       Запись
*   Убедиться, что FLASH_STS_BUSY == 0 (не выполняется никакая другая операция стирания/записи).
*   Разблокировать FLASH_CTRL_OPTWE
*   FLASH_CTRL_OPTPG = 1.
*   Записать слово Option bytes по нужному адресу как *address_in_flash = value;
*   Подождать пока FLASH_STS_BUSY не станет 0.
*   Прочитать Option byte, убедиться что он правильно записан.
*/


#endif /* __N32G45x_H__ */
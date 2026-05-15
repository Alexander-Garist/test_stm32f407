/***********************************************************************************************************************
*   Программирование Flash памяти таргета через программный SWD
***********************************************************************************************************************/

#include "programmer_target_Flash.h"


void Erase_Flash_Page(uint32_t start_address);
void Erase_Flash_All();
void Connect_Target();
void Program_Flash(uint32_t start_address, uint32_t* program_data, uint32_t program_size);
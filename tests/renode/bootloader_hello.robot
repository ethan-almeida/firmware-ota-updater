*** Settings ***
Resource          keywords.resource
Test Setup        Renode.ResetEmulation

*** Test Cases ***
Bootloader Should Print Ready Banner Over UART
    [Tags]    smoke    block1
    Setup STM32F407 Machine    ${BOOTLOADER_ELF}
    Renode.StartEmulation
    Renode.WaitForLineOnUart    [bootloader] ready    timeout=10

Application Image Should Print Banner Over UART
    [Tags]    block1
    Setup STM32F407 Machine    ${APP_ELF}
    Renode.StartEmulation
    Renode.WaitForLineOnUart    [app] hello from the application image    timeout=10

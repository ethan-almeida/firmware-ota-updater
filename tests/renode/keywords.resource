*** Settings ***
Library           String
Library           Remote    ${RENODE_URL}    WITH NAME    Renode

*** Keywords ***
Setup STM32F407 Machine
    [Arguments]    ${elf}
    Renode.Execute Command    mach create "ota"
    Renode.Execute Command    machine LoadPlatformDescription @platforms/cpus/stm32f4.repl
    Renode.Execute Command    sysbus LoadELF "${elf}"
    Renode.CreateTerminalTester    sysbus.usart1

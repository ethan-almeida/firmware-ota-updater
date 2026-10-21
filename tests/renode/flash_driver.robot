*** Settings ***
Resource          keywords.resource
Test Setup        Renode.ResetEmulation

*** Test Cases ***
Flash Driver Erases Programs And Verifies On Real Controller
    [Tags]    block2
    Setup STM32F407 Machine    ${FLASH_TEST_ELF}
    Renode.StartEmulation
    Renode.WaitForLineOnUart    [flashtest] PASS    timeout=15

Flash Contents Survive Reset
    [Tags]    block2
    Setup STM32F407 Machine    ${FLASH_TEST_ELF}
    Renode.StartEmulation
    Renode.WaitForLineOnUart    [flashtest] PASS    timeout=15
    Renode.Execute Command    cpu Reset
    ${value}=    Renode.Execute Command    sysbus ReadDoubleWord 0x080C0000
    ${value}=    Strip String    ${value}
    Should Be Equal As Integers    ${value}    ${{ 0x31474643 }}
    ...    Config record magic at 0x080C0000 after reset
    ${value}=    Renode.Execute Command    sysbus ReadDoubleWord 0x080E0000
    ${value}=    Strip String    ${value}
    Should Be Equal As Integers    ${value}    ${{ 0xA5000000 }}
    ...    First flash test word at 0x080E0000 after reset

*** Settings ***
Resource          keywords.resource
Test Setup        Renode.ResetEmulation

*** Variables ***
${CONFIG_SECTOR}     0x080E0000
${FLASH_CR}          0x40023C10
${FLASH_SR}          0x40023C0C
${FLASH_KEYR}        0x40023C04
${FLASH_LOCK}        ${{ 0x80000000 }}

*** Keywords ***
Create Bare Machine
    Renode.Execute Command    mach create "flash_spike"
    Renode.Execute Command    machine LoadPlatformDescription @platforms/cpus/stm32f4.repl

Register Should Be
    [Arguments]    ${address}    ${expected}    ${msg}
    ${value}=      Renode.Execute Command    sysbus ReadDoubleWord ${address}
    ${value}=      Strip String    ${value}
    Should Be Equal As Integers    ${value}    ${expected}    msg=${msg} (read ${value})

Flash Word Should Be
    [Arguments]    ${address}    ${expected}
    Register Should Be    ${address}    ${expected}
    ...    Flash word at ${address}

Unlock Flash
    Renode.Execute Command    sysbus WriteDoubleWord ${FLASH_KEYR} 0x45670123
    Renode.Execute Command    sysbus WriteDoubleWord ${FLASH_KEYR} 0xCDEF89AB

*** Test Cases ***
Flash Is Locked At Reset And Unlocks Via Key Sequence
    [Tags]    spike    block1
    Create Bare Machine
    Register Should Be    ${FLASH_CR}    ${{ ${FLASH_LOCK} }}
    ...    FLASH_CR at reset
    Unlock Flash
    Register Should Be    ${FLASH_CR}    0
    ...    FLASH_CR after KEYR unlock

Status Register Reads Zero So Busy Polls Terminate
    [Tags]    spike    block1
    Create Bare Machine
    Register Should Be    ${FLASH_SR}    0    FLASH_SR

Direct Bus Writes Reach Flash Regardless Of Lock
    [Tags]    spike    block1
    Create Bare Machine
    ${out}=    Renode.Execute Command    sysbus WriteDoubleWord ${CONFIG_SECTOR} ${{ 0xDEADBEEF }}
    Flash Word Should Be    ${CONFIG_SECTOR}    ${{ 0xDEADBEEF }}
    Unlock Flash
    ${out}=    Renode.Execute Command    sysbus WriteDoubleWord ${CONFIG_SECTOR} ${{ 0x11223344 }}
    Flash Word Should Be    ${CONFIG_SECTOR}    ${{ 0x11223344 }}

Sector Erase Via CR Erases For Real
    [Tags]    spike    block1
    Create Bare Machine
    Unlock Flash
    ${out}=    Renode.Execute Command    sysbus WriteDoubleWord ${CONFIG_SECTOR} ${{ 0xCAFEBABE }}
    Flash Word Should Be    ${CONFIG_SECTOR}    ${{ 0xCAFEBABE }}
    ${out}=    Renode.Execute Command    sysbus WriteDoubleWord ${FLASH_CR} ${{ (1 << 1) | (11 << 3) }}
    Register Should Be    ${FLASH_CR}    ${{ (1 << 1) | (11 << 3) }}
    ...    FLASH_CR erase selection
    ${out}=    Renode.Execute Command    sysbus WriteDoubleWord ${FLASH_CR} ${{ (1 << 16) | (1 << 1) | (11 << 3) }}
    Flash Word Should Be    ${CONFIG_SECTOR}    ${{ 0xFFFFFFFF }}

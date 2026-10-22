*** Settings ***
Resource          keywords.resource
Test Setup        Renode.ResetEmulation

*** Keywords ***
Preload Flash At
    [Arguments]    ${file}    ${address}
    Renode.Execute Command    sysbus LoadBinary "${file}" ${address}

Reboot Machine
    Renode.Execute Command    cpu Reset
    Renode.Execute Command    cpu Start

*** Test Cases ***
Valid Update Boots Confirms And Is Stable Across Reboot
    [Tags]    block4
    Setup STM32F407 Machine    ${BOOTLOADER_ELF}
    Preload Flash At    ${GEN_DIR}/signed_app_a.bin    0x08010000
    Preload Flash At    ${GEN_DIR}/signed_app_b.bin    0x08080000
    Preload Flash At    ${GEN_DIR}/config_pending_b.bin    0x080C0000
    Renode.StartEmulation
    Renode.WaitForLineOnUart    [bootloader] ready    timeout=120
    Renode.WaitForLineOnUart    [bootloader] boot B    timeout=120
    Renode.WaitForLineOnUart    [app] hello from the application image    timeout=30
    Renode.WaitForLineOnUart    [app] update confirmed    timeout=120
    Reboot Machine
    Renode.WaitForLineOnUart    [bootloader] boot B    timeout=120
    Renode.WaitForLineOnUart    [app] hello from the application image    timeout=30

Tampered Update Rejected And Pending Cleared
    [Tags]    block4
    Setup STM32F407 Machine    ${BOOTLOADER_ELF}
    Preload Flash At    ${GEN_DIR}/signed_app_a.bin    0x08010000
    Preload Flash At    ${GEN_DIR}/tampered_app_b.bin    0x08080000
    Preload Flash At    ${GEN_DIR}/config_pending_b.bin    0x080C0000
    Renode.StartEmulation
    Renode.WaitForLineOnUart    [bootloader] ready    timeout=120
    Renode.WaitForLineOnUart    [bootloader] boot A    timeout=120
    Renode.WaitForLineOnUart    [app] hello from the application image    timeout=30
    Reboot Machine
    Renode.WaitForLineOnUart    [bootloader] boot A    timeout=120
    Renode.WaitForLineOnUart    [app] hello from the application image    timeout=30

Three Unconfirmed Boots Roll Back To A
    [Tags]    block4
    Setup STM32F407 Machine    ${BOOTLOADER_ELF}
    Preload Flash At    ${GEN_DIR}/signed_app_a.bin    0x08010000
    Preload Flash At    ${GEN_DIR}/signed_unconfirmed_b.bin    0x08080000
    Preload Flash At    ${GEN_DIR}/config_pending_b.bin    0x080C0000
    Renode.StartEmulation
    Renode.WaitForLineOnUart    [bootloader] ready    timeout=120
    Renode.WaitForLineOnUart    [bootloader] boot B    timeout=120
    Renode.WaitForLineOnUart    [app] running unconfirmed    timeout=30
    Reboot Machine
    Renode.WaitForLineOnUart    [bootloader] boot B    timeout=120
    Renode.WaitForLineOnUart    [app] running unconfirmed    timeout=30
    Reboot Machine
    Renode.WaitForLineOnUart    [bootloader] boot B    timeout=120
    Renode.WaitForLineOnUart    [app] running unconfirmed    timeout=30
    Reboot Machine
    Renode.WaitForLineOnUart    [bootloader] boot A    timeout=120
    Renode.WaitForLineOnUart    [app] hello from the application image    timeout=30
    Reboot Machine
    Renode.WaitForLineOnUart    [bootloader] boot A    timeout=120
    Renode.WaitForLineOnUart    [app] hello from the application image    timeout=30

Old Version Update Rejected And Pending Cleared
    [Tags]    block4
    Setup STM32F407 Machine    ${BOOTLOADER_ELF}
    Preload Flash At    ${GEN_DIR}/signed_app_a.bin    0x08010000
    Preload Flash At    ${GEN_DIR}/old_app_b.bin    0x08080000
    Preload Flash At    ${GEN_DIR}/config_pending_b_minver2.bin    0x080C0000
    Renode.StartEmulation
    Renode.WaitForLineOnUart    [bootloader] ready    timeout=120
    Renode.WaitForLineOnUart    [bootloader] boot A    timeout=120
    Renode.WaitForLineOnUart    [app] hello from the application image    timeout=30
    Reboot Machine
    Renode.WaitForLineOnUart    [bootloader] boot A    timeout=120
    Renode.WaitForLineOnUart    [app] hello from the application image    timeout=30

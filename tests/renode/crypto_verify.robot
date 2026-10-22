*** Settings ***
Resource          keywords.resource
Test Setup        Renode.ResetEmulation

*** Test Cases ***
Python Signed Image Verifies And Tampered Copies Are Rejected On Target
    [Tags]    block3
    Setup STM32F407 Machine    ${CRYPTO_TEST_ELF}
    Renode.StartEmulation
    Renode.WaitForLineOnUart    [cryptotest] valid: OK    timeout=120
    Renode.WaitForLineOnUart    [cryptotest] tampered-payload: OK    timeout=30
    Renode.WaitForLineOnUart    [cryptotest] tampered-signature: OK    timeout=120
    Renode.WaitForLineOnUart    [cryptotest] tampered-digest: OK    timeout=30
    Renode.WaitForLineOnUart    [cryptotest] wrong-key: OK    timeout=120
    Renode.WaitForLineOnUart    [cryptotest] truncated: OK    timeout=30
    Renode.WaitForLineOnUart    [cryptotest] PASS    timeout=30

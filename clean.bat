forfiles /p .vs\ /m ipch /c "cmd /c if @isdir==TRUE rd /s /q @file" /s

rd /s /q "Bins"

rd /s /q "Client\x86"
rd /s /q "Client\x64"

rd /s /q "Server\x86"
rd /s /q "Server\x64"

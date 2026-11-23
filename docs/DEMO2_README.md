# Visioball Demo: Server, Clients en Bluetooth

## Starten

Start Docker Desktop en zet de Visioball aan. Open PowerShell in de workspace-root en start de volledige demo:

```powershell
Push-Location ..\server
.\demo.ps1
Pop-Location
```

Het script start de database en objectopslag, de games-API, exporteert de webclient en start HTTPS en de CA-downloadserver.

## Client op de pc

Open `https://localhost/`. Vertrouw het lokale certificaat eenmalig als de browser daarom vraagt:

```powershell
Import-Certificate -FilePath '..\visiobal-eigen-bijdrage\demo-dist\visioball-local-ca.crt' -CertStoreLocation Cert:\CurrentUser\Root
```

## Mobiele client via hotspot

Verbind de telefoon met de Windows-hotspot van de server-pc. Download en vertrouw de CA eenmalig via `http://192.168.137.1:8083/visioball-local-ca.crt`. Open daarna `https://192.168.137.1/`.

Als het hotspot-IP verandert, gebruik dan het actuele hotspot-IP in beide adressen. De pc-client gebruikt `localhost`; de telefoon gebruikt het IP-adres van de server-pc.

## Bluetooth-demo

Gebruik op de client een browser met Web Bluetooth-ondersteuning, verbind met de Visioball via de app en open daarna **Spellen** om de games uit de API te laden en te starten. Android Chrome ondersteunt Web Bluetooth; iOS Safari niet.

De demo-API is niet geauthenticeerd. Houd deze uitsluitend op een vertrouwd lokaal netwerk.
# CHALLENGE_HUELLAPC
Valida PC corporativo usando las características del ordenador, las claves de registro y una lista de programas abiertos.

Este challenge genera una cadena binaria con un bit por cada una de las características del PC y por cada uno de los programas. 
Las caracterísiticas que se comprueban son las siguientes:
  -  Dominio
  -  Nombre PC
  -  Programas abiertos
  -  Webex
  -  Fuente

En el ejemplo hay 3 programas, por lo tanto la cadena será de 7 bits: Domain, PCName, Program 1, Program 2, Program 3, Webex, Font
La cadena "correcta" es la que quiera el administrador , no necesariamente todo 1s.

La lista de programas abiertos debe especificarse como en el ejemplo:
```json 
{
	"FileName": "CHALLENGE_PC.dll",
	"Description": "This challenge identifies several unique Nokia PC proporties",
	"Props": {
		"validity_time": 3600,
		"refresh_time": 3000,
		"programs":
			[
				"ccSvcHst.exe",
				"ibmpmsvc.exe",
				"SCClient.exe"
			]
	},
	"Requirements": "none"
}
```

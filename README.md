# CHALLENGE_HUELLAPC
Valida PC corporativo usando las características del ordenador, las claves de registro y una lista de programas abiertos.

Este challenge genera un número, siendo cada cifra una de las características del PC. 
Las caracterísiticas que se comprueban son las siguientes:
  -  Dominio
  -  Nombre PC
  -  Programas abiertos
  -  Webex
  -  Fuente

Si las características coinciden con las del administrador, el número será un 1, en caso contrario será un 0.
En el ejemplo hay 4 programas, la cifra podrá ser un número entre 0 y 4, dependiendo de los programas que estén abiertos.
Si los 4 programas del ejemplo están abiertos y el resto de características coiniciden con las especificadas por el administrador en el json el número sería 11411.
La cadena "correcta" es la que quiera el administrador.

La lista de caracterísitcas y programas debe especificarse como en el ejemplo:
```json 
{
	"FileName": "CHALLENGE_huellaPC.dll",
	"Description": "This challenge identifies several unique Nokia PC proporties",
	"Props": {
		"validity_time": 3600,
		"refresh_time": 3000,
		"domain":"nsn-intra.net",
		"sufix":"nsn-intra.net",
		"webex":"nokiameetings",
		"font":"Nokia Pure Headline (TrueType)",
		"programs":
			[
				"csc_ui.exe",
				"Teams.exe",
				"MakeMeAdminService.exe",
				"nxtcoordinator.exe"
			]
	},
	"Requirements": "none"
}
```

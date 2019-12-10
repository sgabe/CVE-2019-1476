# CVE-2019-1476

## AppXSvc Arbitrary File Overwrite DoS

I have independently reported this vulnerability to MSRC as part of my research inspired by [CVE-2019-0841](https://github.com/rogue-kdc/CVE-2019-0841) originally reported by [Nabeel Ahmed](https://twitter.com/rogue_kdc). This vulnerability allows a regular user to overwrite arbitrary files. However, the attacker's capabilities are limited, due to the lack of control over the file's content, hence it's most likely usage is in various denial of service scenarios. See [Arbitrary file overwrite in AppXSvc](https://www.seljan.hu/posts/arbitrary-file-overwrite-in-appxsvc/) for more information.

![Video PoC](https://github.com/sgabe/CVE-2019-1476/blob/master/AppXSvcDoS.gif)
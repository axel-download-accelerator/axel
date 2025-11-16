AXEL - 轻量级 CLI 下载加速器

关于

Axel 尝试通过对每个文件使用多个连接来加速下载过程，并且还可以在不同服务器之间平衡负载。

Axel 试图尽可能轻量，因此它在字节关键型系统上可能很有用。

Axel 支持 HTTP、HTTPS、FTP 和 FTPS 协议。

感谢 Axel 的原始开发者 Wilmer van der Gaast，以及多年来为它做出贡献的其他人。

用法

有关用法信息，请参阅手册页：

如何提供帮助

如果您会编码并且有兴趣改进 Axel，请阅读 CONTRIBUTING.md 文件；如果您在寻找想法，请查看我们的开放工单。

此外，还有一个谷歌群组用于讨论和协调开发。您也可以在 Freenode 的 #axel 频道中找到其他开发人员。

项目的可持续性主要取决于开发人员投入的时间，因此，如果您想贡献但不会编码，也可以通过以下方式资助付费开发时间：

· Ismael Luceno
  · Github 赞助
  · https://c5.patreon.com/external/logo/become_a_patron_button.png
  · https://liberapay.com/assets/widgets/donate.svg

从二进制文件安装

您的操作系统可能包含 Axel 的预编译版本，如果是这样，您可能应该使用它。如果软件包过时，请与软件包维护者联系或在您的发行版中提交支持工单。

从源代码构建

警告：建议仅在开发时从源代码存储库构建，否则请仅使用发布版 tarball。

Axel 使用 GNU 自动工具作为其构建系统；说明在 INSTALL 文件中提供。大多数用户的基本操作是：

要在没有 SSL/TLS 支持的情况下构建，请向 configure 传递 --without-ssl 标志。

如果您是从源代码存储库而不是发布版 tarball 工作，您需要首先使用以下命令生成构建系统：

当从 git 存储库工作时，构建系统将检测到这一点，并在支持的情况下将 -Werror 添加到 CFLAGS 中；因此，如果您不是在进行开发，您可能应该考虑向 configure 传递 --disable-Werror，以防止因轻微警告而导致构建失败。

依赖

· gettext (或 gettext-tiny)
· pkg-config

可选：

· libssl (OpenSSL, LibreSSL 或兼容版本) -- 用于 SSL/TLS 支持。

从快照构建所需的额外依赖

· autoconf-archive
· autoconf
· automake
· autopoint
· txt2man

基于 Debian 的系统上的软件包

· build-essential
· autoconf
· autoconf-archive
· automake
· autopoint
· gettext
· libssl-dev
· pkg-config
· txt2man

Mac OS X (Homebrew) 上的软件包

· autoconf-archive
· automake
· gettext
· openssl

在 Mac OS X (Homebrew) 上构建

您需要向自动工具提供一些额外的选项，以便它可以找到 gettext 和 openssl。

完成这些步骤后，您可以像往常一样运行 make。

相关项目

· aria2
· hget
· lftp
· nugget
· pget

许可证

Axel 根据 GPL-2+ 许可证授权，并带有 OpenSSL 例外。

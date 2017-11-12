class Muon < Formula
  desc "A fast stateless VPN with simple obfuscation"
  homepage "https://github.com/puxxustc/muon"
  head "https://github.com/puxxustc/muon.git"

  depends_on "libmill"
  depends_on "autoconf" => :build
  depends_on "automake" => :build
  depends_on "libtool" => :build

  def install
    system "autoreconf", "-if"
    system "./configure", "--prefix=#{prefix}", "--sysconfdir=#{HOMEBREW_PREFIX}/etc", "--mandir=#{man}"
    system "make"
    system "make install"
  end

  test do
    system "make check"
  end
end

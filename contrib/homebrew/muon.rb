class Muon < Formula
  desc "Simple stateless VPN"
  homepage "https://github.com/XiaoxiaoPu/muon"
  head "https://github.com/XiaoxiaoPu/muon.git"

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
    system "true"
  end
end
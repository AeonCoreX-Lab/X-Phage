# 🧬 X-Phage Titan Official Docker Environment
# OS: Ubuntu 22.04 LTS (Jammy)
FROM ubuntu:22.04

# ১. সিস্টেম ডিপেন্ডেন্সি এবং LLVM ইন্সটল করা
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    clang \
    llvm \
    lld \
    curl \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# ২. ডাইরেক্টরি সেটআপ
WORKDIR /app

# ৩. সোর্স কোড কপি করা
COPY . .

# ৪. লিনাক্সের জন্য নেটিভ কম্পাইল করা
ENV TARGET=linux
RUN bash build.sh

# ৫. গ্লোবাল পাথে সেটআপ করা
RUN mv bin/linux/xphage_linux_x64 /usr/local/bin/xphage
RUN chmod +x /usr/local/bin/xphage

# ৬. ডিফল্ট কমান্ড
ENTRYPOINT ["xphage"]
CMD ["--version"]

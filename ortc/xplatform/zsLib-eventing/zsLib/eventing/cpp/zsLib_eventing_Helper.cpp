/*

Copyright (c) 2016, Robin Raymond
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

*/

#include <zsLib/eventing/internal/zsLib_eventing_Helper.h>

#include <zsLib/Numeric.h>
#include <zsLib/Log.h>
#include <zsLib/Singleton.h>

#include <cstdio>

#include <cryptopp/hex.h>
#include <cryptopp/base64.h>
#include <cryptopp/osrng.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif //_WIN32

namespace zsLib { namespace eventing { ZS_DECLARE_SUBSYSTEM(zsLib_eventing); } }


namespace zsLib
{
  namespace eventing
  {
    using CryptoPP::AutoSeededRandomPool;
    using CryptoPP::Base64Encoder;
    using CryptoPP::Base64Decoder;
    using CryptoPP::ByteQueue;
    using CryptoPP::HexEncoder;
    using CryptoPP::HexDecoder;
    using CryptoPP::StringSink;

    namespace internal
    {
      void installRemoteEventingSettingsDefaults();
      

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------

      class CryptoPPHelper
      {
      public:
        static CryptoPPHelper &singleton()
        {
          AutoRecursiveLock lock(*zsLib::IHelper::getGlobalLock());
          static Singleton<CryptoPPHelper> singleton;
          return singleton.singleton();
        }

        CryptoPPHelper()
        {
          static const char *buffer = "1234567890";

          String result;
          {
            Base64Encoder encoder(new StringSink(result), false);
            encoder.Put((const BYTE *)buffer, strlen(buffer));
            encoder.MessageEnd();
          }

          {
            String &input = result;

            ByteQueue queue;
            queue.Put((BYTE *)input.c_str(), input.size());

            ByteQueue *outputQueue = new ByteQueue;
            Base64Decoder decoder(outputQueue);
            queue.CopyTo(decoder);
            decoder.MessageEnd();
          }
        }

      protected:
        //-----------------------------------------------------------------------
        Log::Params log(const char *message)
        {
          return Log::Params(message, "zsLib::eventing::CryptoPPHelper");
        }
      };

      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      //-----------------------------------------------------------------------
      #pragma mark
      #pragma mark Helper
      #pragma mark

      //-----------------------------------------------------------------------
      Log::Params Helper::slog(const char *message)
      {
        return Log::Params(message, "eventing::Helper");
      }
    } // namespace internal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    #pragma mark
    #pragma mark IHelper
    #pragma mark

    //-------------------------------------------------------------------------
    void IHelper::setup()
    {
      zsLib::IHelper::setup();
      internal::CryptoPPHelper::singleton();
      internal::installRemoteEventingSettingsDefaults();
    }

#ifdef WINRT
    //-------------------------------------------------------------------------
    void IHelper::setup(Windows::UI::Core::CoreDispatcher ^dispatcher)
    {
      zsLib::IHelper::setup(dispatcher);
      internal::CryptoPPHelper::singleton();
      internal::installRemoteEventingSettingsDefaults();
    }
#endif //WINRT

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::loadFile(const char *path) throw (StdError)
    {
      String pathStr(path);
#ifdef _WIN32
      pathStr.replaceAll("/", "\\");
#endif //_WIN32

      FILE *file = NULL;
#ifdef _WIN32
      auto error = fopen_s(&file, pathStr, "rb");
#else
      int error = 0;
      file = fopen(pathStr, "rb");
      if (!file) {
        error = errno;
      }
#endif //_WIN32
      if (0 != error) {
        ZS_THROW_CUSTOM_PROPERTIES_1(StdError, error, String("Failed to read file: ") + pathStr);
      }
      if (NULL == file) return SecureByteBlockPtr();

      fseek(file, 0L, SEEK_END);
      auto size = ftell(file);
      fseek(file, 0L, SEEK_SET);

      auto buffer = make_shared<SecureByteBlock>(size);

      auto read = fread(buffer->BytePtr(), sizeof(BYTE), size, file);

      if (read != static_cast<decltype(read)>(size)) {
        ZS_THROW_CUSTOM_PROPERTIES_1(StdError, ferror(file), String("Failed to read file: ") + pathStr + ", buffer size=" + string(buffer->SizeInBytes()));
      }

      auto result = fclose(file);
      if (0 != result) {
        ZS_THROW_CUSTOM_PROPERTIES_1(StdError, errno, String("Failed to read file: ") + pathStr + ", buffer size=" + string(buffer->SizeInBytes()));
      }

      return buffer;
    }

    //-------------------------------------------------------------------------
    void IHelper::saveFile(const char *path, SecureByteBlock &buffer) throw (StdError)
    {
      String pathStr(path);
#ifdef _WIN32
      pathStr.replaceAll("/", "\\");
#endif //_WIN32

      FILE *file = NULL;
#ifdef _WIN32
      int error = fopen_s(&file, pathStr, "wb");
#else
      int error = 0;
      file = fopen(pathStr, "wb");
      if (!file) {
        error = errno;
      }
#endif //_WIN32
      if (NULL == file) {
        ZS_THROW_CUSTOM_PROPERTIES_1(StdError, error, String("File could not be opened: ") + path);
      }

      auto written = fwrite(buffer.BytePtr(), sizeof(BYTE), buffer.SizeInBytes(), file);
      if (written != buffer.SizeInBytes()) {
        ZS_THROW_CUSTOM_PROPERTIES_1(StdError, ferror(file), String("Failed to write entire file: written=") + string(written) + ", buffer size=" + string(buffer.SizeInBytes()));
      }

      auto result = fclose(file);
      if (0 != result) {
        ZS_THROW_CUSTOM_PROPERTIES_1(StdError, errno, String("Failed to write entire file: written=") + string(written) + ", buffer size=" + string(buffer.SizeInBytes()));
      }
    }

    //-------------------------------------------------------------------------
    void IHelper::mkdir(const char *path, bool ignoreExists) throw (StdError)
    {
      String pathStr(path);
#ifdef _WIN32
      pathStr.replaceAll("/", "\\");
#endif //_WIN32

#ifdef _WIN32
      int error = _mkdir(pathStr);
#else
      int error = ::mkdir(pathStr, S_IRWXU | S_IRWXG | S_IRWXO);
#endif //_WIN32
      if (0 != error) {
        error = errno;
        if (ignoreExists) {
          if (EEXIST == error) return;
        }
        ZS_THROW_CUSTOM_PROPERTIES_1(StdError, error, String("Path could not be created: ") + path);
      }
    }

    //-------------------------------------------------------------------------
    ElementPtr IHelper::read(const SecureByteBlockPtr buffer)
    {
      if (!buffer) return ElementPtr();
      return read(*buffer);
    }

    //-------------------------------------------------------------------------
    ElementPtr IHelper::read(const SecureByteBlock &buffer)
    {
      if (0 == buffer.size()) ElementPtr();
      DocumentPtr doc = Document::createFromAutoDetect(reinterpret_cast<const char *>(buffer.BytePtr()));
      ElementPtr result = doc->getFirstChildElement();
      if (!result) return ElementPtr();
      result->orphan();
      return result;
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::writeJSON(
                                          const Document &doc,
                                          bool prettyPrint
                                          )
    {
      size_t bufferSize = 0;
      std::unique_ptr<char[]> buffer = doc.writeAsJSON(prettyPrint, &bufferSize);
      if (!buffer) return SecureByteBlockPtr();

      return IHelper::convertToBuffer(buffer, bufferSize);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::writeJSON(
                                          DocumentPtr doc,
                                          bool prettyPrint
                                          )
    {
      if (!doc) return SecureByteBlockPtr();
      return writeJSON(*doc, prettyPrint);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::writeXML(const Document &doc)
    {
      size_t bufferSize = 0;
      auto buffer = doc.writeAsXML(&bufferSize);
      if (!buffer) return SecureByteBlockPtr();
      SecureByteBlockPtr result(make_shared<SecureByteBlock>(bufferSize));
      memcpy(result->BytePtr(), &(buffer[0]), bufferSize);
      return result;
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::writeXML(DocumentPtr doc)
    {
      if (!doc) return SecureByteBlockPtr();
      return writeXML(*doc);
    }

    //-------------------------------------------------------------------------
    String IHelper::randomString(size_t lengthInChars)
    {
      static const char *randomCharArray = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
      static size_t randomSize = strlen(randomCharArray);

      BYTE staticBuffer[256];
      char staticOutputBuffer[sizeof(staticBuffer) + 1];

      std::unique_ptr<BYTE[]> allocatedBuffer;
      std::unique_ptr<char[]> allocatedOutputBuffer;

      BYTE *buffer = &(staticBuffer[0]);
      char *output = &(staticOutputBuffer[0]);
      if (lengthInChars > sizeof(staticBuffer)) {
        // use the allocated buffer instead
        allocatedBuffer = std::unique_ptr<BYTE[]>(new BYTE[lengthInChars]);
        allocatedOutputBuffer = std::unique_ptr<char[]>(new char[lengthInChars + 1]);
        buffer = allocatedBuffer.get();
        output = allocatedOutputBuffer.get();
      }

      AutoSeededRandomPool rng;
      rng.GenerateBlock(&(buffer[0]), lengthInChars);

      memset(&(output[0]), 0, sizeof(char)*(lengthInChars + 1));

      for (size_t loop = 0; loop < lengthInChars; ++loop) {
        output[loop] = randomCharArray[((buffer[loop]) % randomSize)];
      }
      return String((CSTR)(&(output[0])));
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::random(size_t lengthInBytes)
    {
      SecureByteBlockPtr output(make_shared<SecureByteBlock>());
      AutoSeededRandomPool rng;
      output->CleanNew(lengthInBytes);
      rng.GenerateBlock(*output, lengthInBytes);
      return output;
    }

    //-------------------------------------------------------------------------
    size_t IHelper::random(size_t minValue, size_t maxValue)
    {
      ZS_THROW_INVALID_ARGUMENT_IF(minValue > maxValue)
        if (minValue == maxValue) return minValue;

      auto range = (maxValue - minValue) + 1;

      decltype(range) value = 0;

      AutoSeededRandomPool rng;
      rng.GenerateBlock((BYTE *)&value, sizeof(value));

      value = minValue + (value % range);

      return value;
    }


    //-------------------------------------------------------------------------
    int IHelper::compare(
                         const SecureByteBlock &left,
                         const SecureByteBlock &right
                         )
    {
      SecureByteBlock::size_type minSize = left.SizeInBytes();
      minSize = (right.SizeInBytes() < minSize ? right.SizeInBytes() : minSize);

      int result = 0;

      if (0 != minSize) {
        result = memcmp(left, right, minSize);
        if (0 != result) return result;
      }

      // they are equal values up to the min size so compare sizes now

      if (left.SizeInBytes() < right.SizeInBytes()) {
        return -1;
      }
      if (right.SizeInBytes() < left.SizeInBytes()) {
        return 1;
      }
      return 0;
    }

    //-------------------------------------------------------------------------
    bool IHelper::isEmpty(SecureByteBlockPtr buffer)
    {
      if (!buffer) return true;
      return (buffer->SizeInBytes() < 1);
    }

    //-------------------------------------------------------------------------
    bool IHelper::isEmpty(const SecureByteBlock &buffer)
    {
      return (buffer.SizeInBytes() < 1);
    }

    //-------------------------------------------------------------------------
    bool IHelper::hasData(SecureByteBlockPtr buffer)
    {
      if (!buffer) return false;
      return (buffer->SizeInBytes() > 0);
    }

    //-------------------------------------------------------------------------
    bool IHelper::hasData(const SecureByteBlock &buffer)
    {
      return (buffer.SizeInBytes() > 0);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::clone(SecureByteBlockPtr pBuffer)
    {
      if (!pBuffer) return SecureByteBlockPtr();
      return IHelper::clone(*pBuffer);
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::clone(const SecureByteBlock &buffer)
    {
      SecureByteBlockPtr pBuffer(make_shared<SecureByteBlock>());
      SecureByteBlock::size_type size = buffer.SizeInBytes();
      if (size < 1) return pBuffer;
      pBuffer->CleanNew(size);

      memcpy(pBuffer->BytePtr(), buffer.BytePtr(), size);
      return pBuffer;
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToString(const SecureByteBlock &buffer)
    {
      if (buffer.size() < 1) return String();
      return (const char *)(buffer.BytePtr());  // return buffer cast as const char *
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertToBuffer(const char *input)
    {
      if (NULL == input) return SecureByteBlockPtr();

      SecureByteBlockPtr output(make_shared<SecureByteBlock>());
      size_t len = strlen(input);
      if (len < 1) return output;

      output->CleanNew(sizeof(char)*len);

      memcpy(*output, input, sizeof(char)*len);
      return output;
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertToBuffer(const std::string &str)
    {
      if (str.size() < 1) return SecureByteBlockPtr();
      auto result(make_shared<SecureByteBlock>(str.length()));
      memcpy(result->BytePtr(), str.c_str(), str.length());
      return result;
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertToBuffer(
                                                const BYTE *buffer,
                                                size_t bufferLengthInBytes
                                                )
    {
      SecureByteBlockPtr output(make_shared<SecureByteBlock>());

      if (bufferLengthInBytes < 1) return output;

      output->CleanNew(bufferLengthInBytes);
      memcpy(*output, buffer, bufferLengthInBytes);
      return output;
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertToBuffer(
                                                const std::unique_ptr<char[]> &arrayStr,
                                                size_t lengthInChars,
                                                bool wipeOriginal
                                                )
    {
      if (!arrayStr.get()) return convertToBuffer((const BYTE *)NULL, 0);

      if (SIZE_MAX == lengthInChars) {
        lengthInChars = strlen(arrayStr.get());
      }

      SecureByteBlockPtr result = convertToBuffer((const BYTE *)(arrayStr.get()), lengthInChars);
      if (wipeOriginal) {
        memset(arrayStr.get(), 0, lengthInChars * sizeof(char));
      }
      return result;
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToBase64(
                                    const BYTE *buffer,
                                    size_t bufferLengthInBytes
                                    )
    {
      internal::CryptoPPHelper::singleton();

      String result;
      Base64Encoder encoder(new StringSink(result), false);
      encoder.Put(buffer, bufferLengthInBytes);
      encoder.MessageEnd();
      return result;
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToBase64(const String &input)
    {
      if (input.isEmpty()) return String();
      return IHelper::convertToBase64((const BYTE *)(input.c_str()), input.length());
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToBase64(const SecureByteBlock &input)
    {
      if (input.size() < 1) return String();
      return IHelper::convertToBase64(input, input.size());
    }

    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertFromBase64(const String &input)
    {
      internal::CryptoPPHelper::singleton();

      SecureByteBlockPtr output(make_shared<SecureByteBlock>());

      ByteQueue queue;
      queue.Put((BYTE *)input.c_str(), input.size());

      ByteQueue *outputQueue = new ByteQueue;
      Base64Decoder decoder(outputQueue);
      queue.CopyTo(decoder);
      decoder.MessageEnd();

      size_t outputLengthInBytes = (size_t)outputQueue->CurrentSize();

      if (outputLengthInBytes < 1) return output;

      output->CleanNew(outputLengthInBytes);

      outputQueue->Get(*output, outputLengthInBytes);
      return output;
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToHex(
                                 const BYTE *buffer,
                                 size_t bufferLengthInBytes,
                                 bool outputUpperCase
                                 )
    {
      String result;

      HexEncoder encoder(new StringSink(result), outputUpperCase);
      encoder.Put(buffer, bufferLengthInBytes);
      encoder.MessageEnd();

      return result;
    }

    //-------------------------------------------------------------------------
    String IHelper::convertToHex(
                                 const SecureByteBlock &input,
                                 bool outputUpperCase
                                 )
    {
      return convertToHex(input, input.size(), outputUpperCase);
    }


    //-------------------------------------------------------------------------
    SecureByteBlockPtr IHelper::convertFromHex(const String &input)
    {
      SecureByteBlockPtr output(make_shared<SecureByteBlock>());
      ByteQueue queue;
      queue.Put((BYTE *)input.c_str(), input.size());

      ByteQueue *outputQueue = new ByteQueue;
      HexDecoder decoder(outputQueue);
      queue.CopyTo(decoder);
      decoder.MessageEnd();

      SecureByteBlock::size_type outputLengthInBytes = (SecureByteBlock::size_type)outputQueue->CurrentSize();
      if (outputLengthInBytes < 1) return output;

      output->CleanNew(outputLengthInBytes);

      outputQueue->Get(*output, outputLengthInBytes);
      return output;
    }

    //-------------------------------------------------------------------------
    String IHelper::getDebugString(
                                   const SecureByteBlock &buffer,
                                   size_t bytesPerGroup,
                                   size_t maxLineLength
                                   )
    {
      return zsLib::IHelper::getDebugString(buffer.BytePtr(), buffer.SizeInBytes(), bytesPerGroup, maxLineLength);
    }

  } // namespace eventing
} // namespace zsLib

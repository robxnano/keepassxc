/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SymmetricCipherStream.h"

SymmetricCipherStream::SymmetricCipherStream(QIODevice* baseDevice)
    : LayeredStream(baseDevice)
    , m_cipher(new SymmetricCipher())
    , m_bufferPos(0)
    , m_bufferFilling(false)
    , m_error(false)
    , m_isInitialized(false)
    , m_dataWritten(false)
    , m_streamCipher(false)
{
}

SymmetricCipherStream::~SymmetricCipherStream()
{
    close();
}

bool SymmetricCipherStream::init(SymmetricCipher::Mode mode,
                                 SymmetricCipher::Direction direction,
                                 const QByteArray& key,
                                 const QByteArray& iv)
{
    m_isInitialized = m_cipher->init(mode, direction, key, iv);
    if (!m_isInitialized) {
        setErrorString(m_cipher->errorString());
        return false;
    }
    m_streamCipher = m_cipher->blockSize(m_cipher->mode()) == 1;
    return true;
}

void SymmetricCipherStream::resetInternalState()
{
    m_buffer.clear();
    m_bufferPos = 0;
    m_bufferFilling = false;
    m_error = false;
    m_dataWritten = false;
    m_cipher->reset();
}

bool SymmetricCipherStream::open(QIODeviceBase::OpenMode mode)
{
    return m_isInitialized && LayeredStream::open(mode);
}

bool SymmetricCipherStream::reset()
{
    if (isWritable() && m_dataWritten) {
        if (!writeBlock(true)) {
            return false;
        }
    }

    resetInternalState();

    return true;
}

void SymmetricCipherStream::close()
{
    if (isWritable() && m_dataWritten) {
        writeBlock(true);
    }

    resetInternalState();

    LayeredStream::close();
}

qint64 SymmetricCipherStream::readData(char* data, qint64 maxSize)
{
    Q_ASSERT(maxSize >= 0);

    if (m_error) {
        return -1;
    }

    qint64 bytesRemaining = maxSize;
    qint64 offset = 0;

    while (bytesRemaining > 0) {
        if ((m_bufferPos == m_buffer.size()) || m_bufferFilling) {
            if (!readBlock()) {
                if (m_error) {
                    return -1;
                } else {
                    return maxSize - bytesRemaining;
                }
            }
        }

        int bytesToCopy = qMin(bytesRemaining, static_cast<qint64>(m_buffer.size() - m_bufferPos));

        memcpy(data + offset, m_buffer.constData() + m_bufferPos, bytesToCopy);

        offset += bytesToCopy;
        m_bufferPos += bytesToCopy;
        bytesRemaining -= bytesToCopy;
    }

    return maxSize;
}

bool SymmetricCipherStream::readBlock()
{
    QByteArray newData;

    if (m_bufferFilling) {
        newData.resize(blockSize() - m_buffer.size());
    } else {
        m_buffer.clear();
        newData.resize(blockSize());
    }

    int readResult = m_baseDevice->read(newData.data(), newData.size());

    if (readResult == -1) {
        m_error = true;
        setErrorString(m_baseDevice->errorString());
        return false;
    } else {
        m_buffer.append(newData.left(readResult));
    }

    if (!m_streamCipher && m_buffer.size() != blockSize()) {
        m_bufferFilling = true;
        return false;
    } else {
        m_bufferPos = 0;
        m_bufferFilling = false;

        if (!m_streamCipher && m_baseDevice->atEnd()) {
            if (!m_cipher->finish(m_buffer)) {
                m_error = true;
                setErrorString(m_cipher->errorString());
                return false;
            }
        } else if (m_buffer.size() > 0) {
            if (!m_cipher->process(m_buffer)) {
                m_error = true;
                setErrorString(m_cipher->errorString());
                return false;
            }
        }
        return m_buffer.size() > 0;
    }
}

qint64 SymmetricCipherStream::writeData(const char* data, qint64 maxSize)
{
    Q_ASSERT(maxSize >= 0);

    if (m_error) {
        return -1;
    }

    m_dataWritten = true;
    qint64 bytesRemaining = maxSize;
    qint64 offset = 0;

    while (bytesRemaining > 0) {
        int bytesToCopy = qMin(bytesRemaining, static_cast<qint64>(blockSize() - m_buffer.size()));

        m_buffer.append(data + offset, bytesToCopy);

        offset += bytesToCopy;
        bytesRemaining -= bytesToCopy;

        if (m_buffer.size() == blockSize()) {
            if (!writeBlock(false)) {
                if (m_error) {
                    return -1;
                } else {
                    return maxSize - bytesRemaining;
                }
            }
        }
    }

    return maxSize;
}

bool SymmetricCipherStream::writeBlock(bool lastBlock)
{
    Q_ASSERT(m_streamCipher || lastBlock || (m_buffer.size() == blockSize()));

    if (lastBlock && !m_streamCipher) {
        QByteArray end;
        if (!m_cipher->finish(m_buffer)) {
            m_error = true;
            setErrorString(m_cipher->errorString());
            return false;
        }
    } else if (!m_cipher->process(m_buffer)) {
        m_error = true;
        setErrorString(m_cipher->errorString());
        return false;
    }

    if (m_baseDevice->write(m_buffer) != m_buffer.size()) {
        m_error = true;
        setErrorString(m_baseDevice->errorString());
        return false;
    } else {
        m_buffer.clear();
        return true;
    }
}

int SymmetricCipherStream::blockSize() const
{
    if (m_streamCipher) {
        return 1024;
    }
    return m_cipher->blockSize(m_cipher->mode());
}

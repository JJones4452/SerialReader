// Quick 'n' Dirty Serial Reader
//MIT License
//
//Copyright(c) 2023 Jake Jones
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

//This is a C++ 20+ project.

#ifdef _MSC_VER
#pragma once
#include <format>
#endif

#ifndef SERIALREADER_H
#define SERIALREADER_H

#ifndef CPP_20_AND_WIN32
#define CPP_20_AND_WIN32 (defined(WIN32) && __cplusplus >= 202002L)
#endif //CPP_20_AND_WIN32

#ifdef CPP_20_AND_WIN32
#include <algorithm>
#include <thread>
#include <vector>
#include <iostream>
#include <memory>

#include <Windows.h>
#ifdef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#endif


/**
 * \brief A quick and easy Serial Reader for COM ports on Windows platforms. This program is an easy to use, C++ 20 piece of software that provides a simple
 * read from COM port. The read process is threaded and controlled with two public functions: StartReading and StopReading. All data in the COM port is read as singular
 * bytes of size char. The program looks for the Line End characters and appends the char array of buffer size 256 (default) with a null terminator
 * before formatting into an std::string and emplacing to a vector. This vector is then easily read via reference for processing.
 */
class SerialReader
{
public:
#ifdef _MSC_VER //Check for MSVC as g++ doesn't support std::format

    SerialReader(const std::string& port_name, const size_t& baud_rate, const size_t& read_data_buffer_size = 256)
        : m_com_port_handler_(INVALID_HANDLE_VALUE), m_com_port_(std::format("\\\\.\\{}", port_name)), m_baud_rate_(baud_rate), m_buffer_size_(read_data_buffer_size)
    {
        std::cout << "I am constructed C++\n";
    }


    SerialReader(const char* port_name, const size_t& baud_rate, const size_t& read_data_buffer_size = 256)
        : m_com_port_handler_(INVALID_HANDLE_VALUE), m_com_port_(std::format("\\\\.\\{}", port_name)), m_baud_rate_(baud_rate), m_buffer_size_(read_data_buffer_size)
    {
        std::cout << "I am constructed C \n";
    }
#else 
    SerialReader(const std::string& port_name, const size_t& baud_rate, const size_t& read_data_buffer_size = 256)
        : m_com_port_handler_(INVALID_HANDLE_VALUE), m_com_port_("\\\\.\\" + port_name), m_baud_rate_(baud_rate), m_buffer_size_(read_data_buffer_size)
    {
        std::cout << "I am constructed C++\n";
    }

    SerialReader(const char* port_name, const size_t& baud_rate, const size_t& read_data_buffer_size = 256)
        : m_com_port_handler_(INVALID_HANDLE_VALUE), m_com_port_("\\\\.\\" + std::string(port_name)), m_baud_rate_(baud_rate), m_buffer_size_(read_data_buffer_size)
    {
        std::cout << "I am constructed C \n";
    }
#endif //_MSC_VER

    ~SerialReader()
    {
        Disconnect();
    }

    [[nodiscard]]
    bool Connect()
    {
        m_com_port_handler_ = CreateFileA(
            m_com_port_.c_str(),
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (m_com_port_handler_ == INVALID_HANDLE_VALUE)
        {
            std::cerr << "Failed to open " << m_com_port_ << "." << std::endl;
            m_is_connected_ = false;
        }

        DCB dcbSerialParams{};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(m_com_port_handler_, &dcbSerialParams))
        {
            std::cerr << "Failed to get the COM port state." << std::endl;
            CloseHandle(m_com_port_handler_);
            m_is_connected_ = false;
        }

        dcbSerialParams.BaudRate = static_cast<DWORD>(m_baud_rate_);  // Set the baud rate (e.g. 9600)
        dcbSerialParams.ByteSize = 8;             // Set the data size (8 bits)
        dcbSerialParams.Parity = NOPARITY;        // Set no parity
        dcbSerialParams.StopBits = ONESTOPBIT;    // Set one stop bit

        if (!SetCommState(m_com_port_handler_, &dcbSerialParams))
        {
            std::cerr << "Failed to set the COM port state." << std::endl;
            CloseHandle(m_com_port_handler_);
            m_is_connected_ = false;
        }
        else
        {
            m_is_connected_ = true;
        }
        return m_is_connected_;
    }

    void Disconnect()
    {
        if (m_com_port_handler_ != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_com_port_handler_);
            m_com_port_handler_ = INVALID_HANDLE_VALUE;
            m_is_connected_ = false;
        }
    }

    /**
     * \brief Sets the user specified end line characters.
     * \param line_end_char std::string for the end line characters. Default in the class is "\r\n".
     */
    void SetLineEndCharacters(const std::string& line_end_char)
    {
        m_line_ending_ = line_end_char;
    }


    bool IsConnected() const
    {
        return m_is_connected_;
    }

    void StartReading(std::atomic<bool>& stop_signal)
    {
        if (!m_is_connected_) return;
        std::thread read_thread(&SerialReader::ReadSerial, this, std::ref(stop_signal));
        read_thread.detach();
    }


    void StopReading(std::atomic<bool>& stop_signal)
    {
        stop_signal = true;
    }

    const std::vector<std::string>& GetImmutableResults() const
    {
        return m_received_data_;
    }

    /**
     * \brief Returns the last inserted string in the reader vector by reference. This should be thread safe as the string is returned by const reference and not modifying the vector.
     * \return Last inserted std::string element in the reader vector.
     */
    const std::string& GetLastElementInVector() const
    {
        return m_received_data_.back();
    }

private:
    HANDLE m_com_port_handler_;
    const std::string m_com_port_;
    const size_t m_baud_rate_ = 9600;
    const size_t m_buffer_size_ = 256;
    bool m_is_connected_ = false;
    std::string m_line_ending_ = "\r\n";
    std::vector<std::string> m_received_data_;
    bool m_is_thread_running_ = false;

    /**
     * \brief The primary function for reading from the Serial port. This is spun up in a seperate thread by the "StartReading() function. It looks for
     * bytes returning from the ReadFile function and receives a single byte at a time. Inside, it checks the bytes for the end line character and once the condition
     * is met, the char array of buffer size specified in the constructor is null terminated, converted to an std::string and emplaced into the received data vector.
     *
     * This is memory safe, the char array is initialised as a unique_ptr so there is no manual memory management here.
     *
     * If no bytes are received. The function terminates.
     * \param stop_signal Atomic bool to control the thread that spins the read up.
     */
    void ReadSerial(std::atomic<bool>& stop_signal)
    {
        if (!m_is_connected_) return;
        std::unique_ptr<char[]> tmp_arr = std::make_unique_for_overwrite<char[]>(m_buffer_size_);

        char data;
        DWORD bytes_read;

        short num_end_chars_found = 0;
        size_t iter_for_arr = 0;

        if (!ReadFile(m_com_port_handler_, &data, sizeof(data), &bytes_read, NULL))
        {
            std::cerr << "Failed to read from the COM port." << std::endl;
            return;
        }

        while (!stop_signal && ReadFile(m_com_port_handler_, &data, sizeof(char), &bytes_read, NULL))
        {
            if (bytes_read > 0)
            {
                if (std::any_of(m_line_ending_.begin(), m_line_ending_.end(), [data](const char& c) { return data == c; }))
                {
                    num_end_chars_found++;
                    if (num_end_chars_found == static_cast<short>(m_line_ending_.size()))
                    {
                        tmp_arr[iter_for_arr] = '\0';
                        m_received_data_.emplace_back(tmp_arr.get());

                        num_end_chars_found = 0;
                        iter_for_arr = 0;
                    }
                    continue;
                }
                tmp_arr[iter_for_arr] = data;
                iter_for_arr++;
            }
            else
            {
                break;
            }
        }
    }
};
#endif // CPP_20_AND_WIN32
#endif // SERIALREADER_H
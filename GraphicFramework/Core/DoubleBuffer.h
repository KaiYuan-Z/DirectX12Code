#pragma once

template <class BufferType>
class TDoubleBuffer
{
public:
	TDoubleBuffer() 
	{ 
		m_pFrontBuffer = &m_Buffer0;
		m_pBackBuffer = &m_Buffer1;
	}

	void SwapBuffer() 
	{
		std::swap(m_pFrontBuffer, m_pBackBuffer);
	}

	BufferType& GetFrontBuffer() { return *m_pFrontBuffer; }
	BufferType& GetBackBuffer() { return *m_pBackBuffer; }

private:
	BufferType m_Buffer0;
	BufferType m_Buffer1;
	BufferType* m_pFrontBuffer = nullptr;
	BufferType* m_pBackBuffer = nullptr;
};
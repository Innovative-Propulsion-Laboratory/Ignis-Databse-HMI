#include "data_buffer.h"

void DataBuffer::push(const DataRecord& record) //permet d'ajouter un enregistrement au buffer
{
    m_records.append(record);
}

bool DataBuffer::isFull() const //vérifie si le buffer a atteint sa capacité maximale
{
    return m_records.size() >= BUFFER_SIZE;
}

int DataBuffer::size() const //retourne le nombre d'enregistrements actuellement dans le buffer
{
    return m_records.size();
}

void DataBuffer::clear() //vide le buffer en supprimant tous les enregistrements
{
    m_records.clear();
}

const QVector<DataRecord>& DataBuffer::records() const //permettde lire les données sans les modifier (en retournant une référence constante au vecteur d'enregistrements)
{
    return m_records;
}

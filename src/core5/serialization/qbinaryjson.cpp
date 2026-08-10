// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qbinaryjson_p.h"

#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qvarlengtharray.h>

#include <private/qbinaryjsonarray_p.h>
#include <private/qbinaryjsonobject_p.h>

QT_BEGIN_NAMESPACE

/*!
    \namespace QBinaryJson
    \inmodule QtCore5Compat
    \brief Contains functions for converting QJsonDocument to and from JSON binary format.

    This namespace provides utility functions to keep compatibility with
    older code, which uses the JSON binary format for serializing JSON. Qt JSON
    types can be converted to Qt CBOR types, which can in turn be serialized
    into the CBOR binary format and vice versa.
*/

/*! \enum QBinaryJson::DataValidation

    This enum is used to tell QJsonDocument whether to validate the binary data
    when converting to a QJsonDocument using fromBinaryData() or fromRawData().

    \value Validate Validate the data before using it. This is the default.
    \value BypassValidation Bypasses data validation. Only use if you received the
    data from a trusted place and know it's valid, as using of invalid data can crash
    the application.
*/

namespace QBinaryJson {

/*!
    Creates a QJsonDocument that uses the first \a size bytes from
    \a data. It assumes \a data contains a binary encoded JSON document.
    The created document does not take ownership of \a data. The data is
    copied into a different data structure, and the original data can be
    deleted or modified afterwards.

    \a data has to be aligned to a 4 byte boundary.

    \a validation decides whether the data is checked for validity before being used.
    By default the data is validated. If the \a data is not valid, the method returns
    a null document.

    Returns a QJsonDocument representing the data.

    \note The binary JSON encoding is only retained for backwards
    compatibility. It is undocumented and restrictive in the maximum size of JSON
    documents that can be encoded. Qt JSON types can be converted to Qt CBOR types,
    which can in turn be serialized into the CBOR binary format and vice versa. The
    CBOR format is a well-defined and less restrictive binary representation for a
    superset of JSON.

    \note Before Qt 5.15, the caller had to guarantee that \a data would not be
    deleted or modified as long as any QJsonDocument, QJsonObject or QJsonArray
    still referenced the data. From Qt 5.15 on, this is not necessary anymore.

    \sa toRawData(), fromBinaryData(), DataValidation, QCborValue
*/
QJsonDocument fromRawData(const char *data, int size, DataValidation validation)
{
    if (quintptr(data) & 3) {
        qWarning("QJsonDocument::fromRawData: data has to have 4 byte alignment");
        return QJsonDocument();
    }

    if (size < 0 || uint(size) < sizeof(QBinaryJsonPrivate::Header) + sizeof(QBinaryJsonPrivate::Base))
        return QJsonDocument();

    std::unique_ptr<QBinaryJsonPrivate::ConstData> binaryData
            = std::make_unique<QBinaryJsonPrivate::ConstData>(data, size);

    return (validation == BypassValidation || binaryData->isValid())
            ? binaryData->toJsonDocument()
            : QJsonDocument();
}

static char *toRawDataHelper(const QJsonDocument &document, int *size)
{
    if (document.isNull()) {
        *size = 0;
        return nullptr;
    }

    char *rawData = nullptr;
    uint rawDataSize = 0;
    if (document.isObject()) {
        QBinaryJsonObject o = QBinaryJsonObject::fromJsonObject(document.object());
        rawData = o.takeRawData(&rawDataSize);
    } else {
        QBinaryJsonArray a = QBinaryJsonArray::fromJsonArray(document.array());
        rawData = a.takeRawData(&rawDataSize);
    }

    // It would be quite miraculous if not, as we should have hit the 128MB limit then.
    Q_ASSERT(rawDataSize <= uint(std::numeric_limits<int>::max()));

    *size = static_cast<int>(rawDataSize);
    return rawData;
}

/*!
    Returns the raw binary representation of \a document.
    \a size will contain the size of the returned data.

    This method is useful to e.g. stream the JSON document
    in its binary form to a file.

    \warning In Qt 6, unlike Qt 5, the caller takes ownership of the returned
    data and must release it with \c{free()} once it is not needed anymore.
    Use toBinaryData() to get the binary representation in a QByteArray, which
    manages the memory automatically.

    \note The binary JSON encoding is only retained for backwards
    compatibility. It is undocumented and restrictive in the maximum size of JSON
    documents that can be encoded. Qt JSON types can be converted to Qt CBOR types,
    which can in turn be serialized into the CBOR binary format and vice versa. The
    CBOR format is a well-defined and less restrictive binary representation for a
    superset of JSON.

    \sa fromRawData(), fromBinaryData(), toBinaryData(), QCborValue
*/
const char *toRawData(const QJsonDocument &document, int *size)
{
    qWarning("QBinaryJson: In Qt 6, unlike Qt 5, toRawData() transfers ownership "
             "of the pointer to the caller. Prefer toBinaryData() instead.");
    return toRawDataHelper(document, size);
}

/*!
    Creates a QJsonDocument from \a data.

    \a validation decides whether the data is checked for validity before being used.
    By default the data is validated. If the \a data is not valid, the method returns
    a null document.

    \note The binary JSON encoding is only retained for backwards
    compatibility. It is undocumented and restrictive in the maximum size of JSON
    documents that can be encoded. Qt JSON types can be converted to Qt CBOR types,
    which can in turn be serialized into the CBOR binary format and vice versa. The
    CBOR format is a well-defined and less restrictive binary representation for a
    superset of JSON.

    \sa toBinaryData(), fromRawData(), DataValidation, QCborValue
*/
QJsonDocument fromBinaryData(const QByteArray &data, DataValidation validation)
{
    if (size_t(data.size()) < sizeof(QBinaryJsonPrivate::Header) + sizeof(QBinaryJsonPrivate::Base))
        return QJsonDocument();

    QBinaryJsonPrivate::Header h;
    memcpy(&h, data.constData(), sizeof(QBinaryJsonPrivate::Header));
    QBinaryJsonPrivate::Base root;
    memcpy(&root, data.constData() + sizeof(QBinaryJsonPrivate::Header),
           sizeof(QBinaryJsonPrivate::Base));

    uint size = 0;
    if (qAddOverflow(uint{sizeof(QBinaryJsonPrivate::Header)}, uint{root.size}, &size))
        return QJsonDocument(); // not representable in Private::ConstData

    if (h.tag != QJsonDocument::BinaryFormatTag || h.version != 1U || size > size_t(data.size()))
        return QJsonDocument();

    std::unique_ptr<QBinaryJsonPrivate::ConstData> d
            = std::make_unique<QBinaryJsonPrivate::ConstData>(data.constData(), size);

    return (validation == BypassValidation || d->isValid())
            ? d->toJsonDocument()
            : QJsonDocument();
}

/*!
    Returns a binary representation of \a document.

    The binary representation is also the native format used internally in Qt,
    and is very efficient and fast to convert to and from.

    The binary format can be stored on disk and interchanged with other applications
    or computers. fromBinaryData() can be used to convert it back into a
    JSON document.

    \note The binary JSON encoding is only retained for backwards
    compatibility. It is undocumented and restrictive in the maximum size of JSON
    documents that can be encoded. Qt JSON types can be converted to Qt CBOR types,
    which can in turn be serialized into the CBOR binary format and vice versa. The
    CBOR format is a well-defined and less restrictive binary representation for a
    superset of JSON.

    \sa fromBinaryData(), QCborValue
*/
QByteArray toBinaryData(const QJsonDocument &document)
{
    int size = 0;
    char *raw = toRawDataHelper(document, &size);
    // We have to cleanup the raw data, because QBA makes a copy of it.
    const auto releaseRawData = qScopeGuard([raw]() {
        free(raw);
    });
    return QByteArray(raw, size);
}

} // namespace QBinaryJson

namespace QBinaryJsonPrivate {

static Q_CONSTEXPR Base emptyArray  = {
    { qle_uint(sizeof(Base)) },
    { 0 },
    { qle_uint(0) }
};

static Q_CONSTEXPR Base emptyObject = {
    { qle_uint(sizeof(Base)) },
    { qToLittleEndian(1U) },
    { qle_uint(0) }
};

void MutableData::compact()
{
    static_assert(sizeof(Value) == sizeof(offset));

    Base *base = header->root();
    int reserve = 0;
    if (base->isObject()) {
        auto *o = static_cast<Object *>(base);
        for (uint i = 0; i < o->length(); ++i)
            reserve += o->entryAt(i)->usedStorage(o);
    } else {
        auto *a = static_cast<Array *>(base);
        for (uint i = 0; i < a->length(); ++i)
            reserve += a->at(i)->usedStorage(a);
    }

    uint size = sizeof(Base) + reserve + base->length() * sizeof(offset);
    uint alloc = sizeof(Header) + size;
    auto *h = reinterpret_cast<Header *>(malloc(alloc));
    Q_CHECK_PTR(h);
    h->tag = QJsonDocument::BinaryFormatTag;
    h->version = 1;
    Base *b = new (h->root()) Base{};
    b->size = size;
    if (header->root()->isObject())
        b->setIsObject();
    else
        b->setIsArray();
    b->setLength(base->length());
    b->tableOffset = reserve + sizeof(Array);

    uint offset = sizeof(Base);
    if (b->isObject()) {
        const auto *o = static_cast<const Object *>(base);
        auto *no = static_cast<Object *>(b);

        for (uint i = 0; i < o->length(); ++i) {
            no->table()[i] = offset;

            const Entry *e = o->entryAt(i);
            Entry *ne = no->entryAt(i);
            uint s = e->size();
            memcpy(ne, e, s);
            offset += s;
            uint dataSize = e->value.usedStorage(o);
            if (dataSize) {
                memcpy(reinterpret_cast<char *>(no) + offset, e->value.data(o), dataSize);
                ne->value.setValue(offset);
                offset += dataSize;
            }
        }
    } else {
        const auto *a = static_cast<const Array *>(base);
        auto *na = static_cast<Array *>(b);

        for (uint i = 0; i < a->length(); ++i) {
            const Value *v = a->at(i);
            Value *nv = na->at(i);
            *nv = *v;
            uint dataSize = v->usedStorage(a);
            if (dataSize) {
                memcpy(reinterpret_cast<char *>(na) + offset, v->data(a), dataSize);
                nv->setValue(offset);
                offset += dataSize;
            }
        }
    }
    Q_ASSERT(offset == uint(b->tableOffset));

    free(header);
    header = h;
    this->alloc = alloc;
    compactionCounter = 0;
}

bool ConstData::isValid() const
{
    if (header->tag != QJsonDocument::BinaryFormatTag || header->version != 1U)
        return false;

    const Base *root = header->root();
    const uint maxSize = alloc - sizeof(Header);

    IsValidStack stack;
    stack.push_back(IsValidFrame(root, maxSize));
    while (!stack.isEmpty()) {
        const auto [base, size] = stack.last();
        stack.removeLast();
        const bool res = base->isObject()
                ? static_cast<const Object *>(base)->isValidHelper(size, stack)
                : static_cast<const Array *>(base)->isValidHelper(size, stack);
        if (!res)
            return false;
    }
    return true;
}

QJsonDocument ConstData::toJsonDocument() const
{
    DocumentStack stack;
    stack.push_back(DocumentFrame(header->root()));

    while (true) {
        DocumentFrame &frame = stack.last();

        if (frame.index < frame.base->length()) {
            const uint i = frame.index++;

            const Value *value = nullptr;
            QString key;
            if (frame.isObject) {
                const Entry *e = static_cast<const Object *>(frame.base)->entryAt(i);
                value = &e->value;
                key = e->key();
            } else {
                value = static_cast<const Array *>(frame.base)->at(i);
            }

            const uint type = value->type();
            if (type == QJsonValue::Array || type == QJsonValue::Object) {
                const Base *child = value->base(frame.base);
                // This may reallocate, so frame must not be touched afterwards.
                stack.push_back(DocumentFrame(child, std::move(key)));
                continue;
            }

            const QJsonValue scalar = value->toScalarJsonValue(frame.base);
            if (frame.isObject)
                frame.object.insert(key, scalar);
            else
                frame.array.append(scalar);
            continue;
        }

        // This container is complete: hand it to its parent, or return it.
        if (stack.size() == 1) {
            return frame.isObject ? QJsonDocument(std::move(frame.object))
                                  : QJsonDocument(std::move(frame.array));
        } else {
            const QJsonValue val = frame.isObject ? QJsonValue(std::move(frame.object))
                                                  : QJsonValue(std::move(frame.array));
            const QString key = std::move(frame.key);
            stack.removeLast();

            DocumentFrame &parent = stack.last();
            if (parent.isObject)
                parent.object.insert(key, val);
            else
                parent.array.append(val);
        }
    }
}

uint Base::reserveSpace(uint dataSize, uint posInTable, uint numItems, bool replace)
{
    Q_ASSERT(posInTable <= length());
    if (size + dataSize >= Value::MaxSize) {
        qWarning("QJson: Document too large to store in data structure %d %d %d",
                 uint(size), dataSize, Value::MaxSize);
        return 0;
    }

    offset off = tableOffset;
    // move table to new position
    if (replace) {
        memmove(reinterpret_cast<char *>(table()) + dataSize, table(), length() * sizeof(offset));
    } else {
        memmove(reinterpret_cast<char *>(table() + posInTable + numItems) + dataSize,
                table() + posInTable, (length() - posInTable) * sizeof(offset));
        memmove(reinterpret_cast<char *>(table()) + dataSize, table(), posInTable * sizeof(offset));
    }
    tableOffset += dataSize;
    for (uint i = 0; i < numItems; ++i)
        table()[posInTable + i] = off;
    size += dataSize;
    if (!replace) {
        setLength(length() + numItems);
        size += numItems * sizeof(offset);
    }
    return off;
}

uint Object::indexOf(QStringView key, bool *exists) const
{
    uint min = 0;
    uint n = length();
    while (n > 0) {
        uint half = n >> 1;
        uint middle = min + half;
        if (*entryAt(middle) >= key) {
            n = half;
        } else {
            min = middle + 1;
            n -= half + 1;
        }
    }
    if (min < length() && *entryAt(min) == key) {
        *exists = true;
        return min;
    }
    *exists = false;
    return min;
}

bool Object::isValidHelper(uint maxSize, IsValidStack &stack) const
{
    if (!isAlignedOffset(tableOffset))
        return false;

    if (size > maxSize || tableOffset + length() * sizeof(offset) > size)
        return false;

    QString lastKey;
    for (uint i = 0; i < length(); ++i) {
        const uint entryOffset = table()[i];
        if (!isAlignedOffset(entryOffset) || entryOffset + sizeof(Entry) >= tableOffset)
            return false;
        const Entry *e = entryAt(i);
        if (!e->isValid(tableOffset - entryOffset))
            return false;
        const QString key = e->key();
        if (key < lastKey)
            return false;
        if (!e->value.isValidHelper(this, stack))
            return false;
        lastKey = key;
    }
    return true;
}

bool Array::isValidHelper(uint maxSize, IsValidStack &stack) const
{
    if (!isAlignedOffset(tableOffset))
        return false;

    if (size > maxSize || tableOffset + length() * sizeof(offset) > size)
        return false;

    const offset *values = table();
    for (uint i = 0; i < length(); ++i) {
        if (!reinterpret_cast<const Value *>(values + i)->isValidHelper(this, stack))
            return false;
    }
    return true;
}

uint Value::usedStorage(const Base *b) const
{
    uint s = 0;
    switch (type()) {
    case QJsonValue::Double:
        if (!isLatinOrIntValue())
            s = sizeof(double);
        break;
    case QJsonValue::String: {
        const char *d = data(b);
        s = isLatinOrIntValue()
                ? (sizeof(ushort)
                   + qFromLittleEndian(*reinterpret_cast<const ushort *>(d)))
                : (sizeof(int)
                   + sizeof(ushort) * qFromLittleEndian(*reinterpret_cast<const int *>(d)));
        break;
    }
    case QJsonValue::Array:
    case QJsonValue::Object:
        s = base(b)->size;
        break;
    case QJsonValue::Null:
    case QJsonValue::Bool:
    default:
        break;
    }
    return alignedSize(s);
}

QJsonValue Value::toScalarJsonValue(const Base *b) const
{
    Q_ASSERT(type() != QJsonValue::Array && type() != QJsonValue::Object);

    switch (type()) {
    case QJsonValue::Null:
        return QJsonValue(QJsonValue::Null);
    case QJsonValue::Bool:
        return QJsonValue(toBoolean());
    case QJsonValue::Double:
        return QJsonValue(toDouble(b));
    case QJsonValue::String:
        return QJsonValue(toString(b));
    default:
        // This handled Undefined, and the out-of-range type codes 6 and 7,
        // which fit the three bit type field.
        // We should handle them gracefully, rather than trigger Q_UNREACHABLE()
        return QJsonValue(QJsonValue::Undefined);
    }
}

inline bool isValidValueOffset(uint offset, uint tableOffset)
{
    return offset >= sizeof(Base)
        && isAlignedOffset(offset)
        && offset + sizeof(uint) <= tableOffset;
}

bool Value::isValidHelper(const Base *b, IsValidStack &stack) const
{
    switch (type()) {
    case QJsonValue::Null:
    case QJsonValue::Bool:
        return true;
    case QJsonValue::Double:
        return isLatinOrIntValue() || isValidValueOffset(value(), b->tableOffset);
    case QJsonValue::String:
        if (!isValidValueOffset(value(), b->tableOffset))
            return false;
        if (isLatinOrIntValue())
            return asLatin1String(b).isValid(b->tableOffset - value());
        return asString(b).isValid(b->tableOffset - value());
    case QJsonValue::Array:
    case QJsonValue::Object:
        // do not recurse, instead only check the offset and push the nested
        // object check on the stack
        if (isValidValueOffset(value(), b->tableOffset)) {
            stack.push_back(IsValidFrame(base(b), b->tableOffset - value()));
            return true;
        } else {
            return false;
        }
    default:
        return false;
    }
}

uint Value::requiredStorage(const QBinaryJsonValue &v, bool *compressed)
{
    *compressed = false;
    switch (v.type()) {
    case QJsonValue::Double:
        if (QBinaryJsonPrivate::compressedNumber(v.toDouble()) != INT_MAX) {
            *compressed = true;
            return 0;
        }
        return sizeof(double);
    case QJsonValue::String: {
        QString s = v.toString();
        *compressed = QBinaryJsonPrivate::useCompressed(s);
        return QBinaryJsonPrivate::qStringSize(s, *compressed);
    }
    case QJsonValue::Array:
    case QJsonValue::Object:
        return v.base ? uint(v.base->size) : sizeof(QBinaryJsonPrivate::Base);
    case QJsonValue::Undefined:
    case QJsonValue::Null:
    case QJsonValue::Bool:
        break;
    }
    return 0;
}

uint Value::valueToStore(const QBinaryJsonValue &v, uint offset)
{
    switch (v.type()) {
    case QJsonValue::Undefined:
    case QJsonValue::Null:
        break;
    case QJsonValue::Bool:
        return v.toBool();
    case QJsonValue::Double: {
        int c = QBinaryJsonPrivate::compressedNumber(v.toDouble());
        if (c != INT_MAX)
            return c;
    }
        Q_FALLTHROUGH();
    case QJsonValue::String:
    case QJsonValue::Array:
    case QJsonValue::Object:
        return offset;
    }
    return 0;
}

void Value::copyData(const QBinaryJsonValue &v, char *dest, bool compressed)
{
    switch (v.type()) {
    case QJsonValue::Double:
        if (!compressed)
            qToLittleEndian(v.toDouble(), dest);
        break;
    case QJsonValue::String: {
        const QString str = v.toString();
        QBinaryJsonPrivate::copyString(dest, str, compressed);
        break;
    }
    case QJsonValue::Array:
    case QJsonValue::Object: {
        const QBinaryJsonPrivate::Base *b = v.base;
        if (!b)
            b = (v.type() == QJsonValue::Array ? &emptyArray : &emptyObject);
        memcpy(dest, b, b->size);
        break;
    }
    default:
        break;
    }
}

} // namespace QBinaryJsonPrivate

QT_END_NAMESPACE

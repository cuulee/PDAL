/******************************************************************************
* Copyright (c) 2011, Michael P. Gerlek (mpg@flaxen.com)
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/

#include <cassert>

#include "libpc/LasWriter.hpp"
#include "libpc/LasHeaderWriter.hpp"

namespace libpc
{


LasWriter::LasWriter(Stage& prevStage, std::ostream& ostream)
    : Writer(prevStage)
    , m_ostream(ostream)
{
    LasHeader* lasHeader = new LasHeader;
    setHeader(lasHeader);

    return;
}


void LasWriter::writeBegin(std::size_t totalNumPoints)
{
    Header& baseHeader = getHeader();
    LasHeader& lasHeader = (LasHeader&)baseHeader;

    // need to set properties of the header here, based on this->getHeader() and on the user's preferences
    lasHeader.setBounds( baseHeader.getBounds() );
    lasHeader.SetOffset(0,0,0);
    lasHeader.SetScale(1,1,1);
    
    boost::uint32_t cnt = static_cast<boost::uint32_t>(totalNumPoints);
    assert(cnt==totalNumPoints);
    lasHeader.SetPointRecordsCount(cnt);

    LasHeaderWriter lasHeaderWriter(lasHeader, m_ostream);
    lasHeaderWriter.write();

    return;
}


void LasWriter::writeEnd()
{
    return;
}


template<class T>
static inline void write_field(T v, boost::uint8_t*& p)
{
    *(T*)p = v;
    p += sizeof(T);
    return;
}


void LasWriter::writeBuffer(const PointData& pointData)
{
    Header& baseHeader = getHeader();
    LasHeader& lasHeader = (LasHeader&)baseHeader;

    const Schema& schema = pointData.getSchema();
    LasHeader::PointFormatId pointFormat = lasHeader.getDataFormatId();

    const std::size_t fieldIndexX = schema.getDimensionIndex("X");
    const std::size_t fieldIndexY = schema.getDimensionIndex("Y");
    const std::size_t fieldIndexZ = schema.getDimensionIndex("Z");
    const std::size_t fieldIndexIntensity = schema.getDimensionIndex("Intensity");
    const std::size_t fieldIndexReturnNum = schema.getDimensionIndex("Return Number");
    const std::size_t fieldIndexNumReturns = schema.getDimensionIndex("Number of Returns");
    const std::size_t fieldIndexScanDir = schema.getDimensionIndex("Scan Direction");
    const std::size_t fieldIndexFlight = schema.getDimensionIndex("Flightline Edge");
    const std::size_t fieldIndexClassification = schema.getDimensionIndex("Classification");
    const std::size_t fieldIndexScanAngle = schema.getDimensionIndex("Scan Angle Rank");
    const std::size_t fieldIndexUserData = schema.getDimensionIndex("User Data");
    const std::size_t fieldIndexPointSource = schema.getDimensionIndex("Point Source ID");
    const std::size_t fieldIndexTime = schema.getDimensionIndex("Time");
    const std::size_t fieldIndexRed = schema.getDimensionIndex("Red");
    const std::size_t fieldIndexGreen = schema.getDimensionIndex("Green");
    const std::size_t fieldIndexBlue = schema.getDimensionIndex("Blue");

    for (boost::uint32_t pointIndex=0; pointIndex<pointData.getNumPoints(); pointIndex++)
    {
        boost::uint8_t buf[100];

        if (pointFormat == LasHeader::ePointFormat0)
        {
            Utils::write_n(m_ostream, buf, LasHeader::ePointSize0);
        }
        else if (pointFormat == LasHeader::ePointFormat1)
        {
            Utils::write_n(m_ostream, buf, LasHeader::ePointSize1);
        }
        else if (pointFormat == LasHeader::ePointFormat2)
        {
            Utils::write_n(m_ostream, buf, LasHeader::ePointSize2);
        }
        else if (pointFormat == LasHeader::ePointFormat3)
        {
            boost::uint8_t* p = buf;

            const boost::uint32_t x = pointData.getField<boost::uint32_t>(pointIndex, fieldIndexX);
            const boost::uint32_t y = pointData.getField<boost::uint32_t>(pointIndex, fieldIndexY);
            const boost::uint32_t z = pointData.getField<boost::uint32_t>(pointIndex, fieldIndexZ);
            const boost::uint16_t intensity = pointData.getField<boost::uint16_t>(pointIndex, fieldIndexIntensity);
            
            const boost::uint8_t returnNum = pointData.getField<boost::uint8_t>(pointIndex, fieldIndexReturnNum);
            const boost::uint8_t numReturns = pointData.getField<boost::uint8_t>(pointIndex, fieldIndexNumReturns);
            const boost::uint8_t scanDirFlag = pointData.getField<boost::uint8_t>(pointIndex, fieldIndexScanDir);
            const boost::uint8_t flight = pointData.getField<boost::uint8_t>(pointIndex, fieldIndexFlight);

            const boost::uint8_t bits = returnNum & (numReturns<<3) & (scanDirFlag << 6) && (flight << 7);

            const boost::uint8_t classification = pointData.getField<boost::uint8_t>(pointIndex, fieldIndexClassification);
            const boost::int8_t scanAngleRank = pointData.getField<boost::int8_t>(pointIndex, fieldIndexScanAngle);
            const boost::uint8_t user = pointData.getField<boost::uint8_t>(pointIndex, fieldIndexUserData);
            const boost::uint16_t pointSourceId = pointData.getField<boost::uint16_t>(pointIndex, fieldIndexPointSource);
            const double gpsTime = pointData.getField<double>(pointIndex, fieldIndexTime);
            const boost::uint16_t red = pointData.getField<boost::uint16_t>(pointIndex, fieldIndexRed);
            const boost::uint16_t green = pointData.getField<boost::uint16_t>(pointIndex, fieldIndexGreen);
            const boost::uint16_t blue = pointData.getField<boost::uint16_t>(pointIndex, fieldIndexBlue);


            write_field<boost::uint32_t>(x, p);
            write_field<boost::uint32_t>(y, p);
            write_field<boost::uint32_t>(z, p);
            write_field<boost::uint16_t>(intensity, p);
            write_field<boost::uint8_t>(bits, p);
            write_field<boost::uint8_t>(classification, p);
            write_field<boost::int8_t>(scanAngleRank, p);
            write_field<boost::uint8_t>(user, p);
            write_field<boost::uint16_t>(pointSourceId, p);
            write_field<double>(gpsTime, p);
            write_field<boost::uint16_t>(red, p);
            write_field<boost::uint16_t>(green, p);
            write_field<boost::uint16_t>(blue, p);

            Utils::write_n(m_ostream, buf, LasHeader::ePointSize3);
        }
        else
        {
            throw;
        }
    }


    //std::vector<boost::uint8_t> const& data = point.GetData();    
    //detail::write_n(m_ofs, data.front(), m_header->GetDataRecordLength());

    return;
}

} // namespace libpc
